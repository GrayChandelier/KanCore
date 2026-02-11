#include "../include/Window.hpp"

namespace KanCore::Graphics
{

	// -------------------- Window --------------------
	void Window::initialize(const std::string& title, const WindowPreset& preset)
	{
		//initialize GLFW once
		WindowDetails::initializeLibraries();

		glfwWindow = glfwCreateWindow(
			preset.window.width,
			preset.window.height,
			title.c_str(),
			preset.monitor,
			preset.shareWith
		);

		if (!glfwWindow)
			throw std::runtime_error("Failed to create GLFW window");

		properties.setTitle(title);
		properties.setVisible(preset.window.visible);
		properties.setResizable(preset.window.resizable);
		properties.setDecorated(preset.window.decorated);
	
		context.makeCurrent();
		context.setVSync(preset.graphics.vsync);


		static std::atomic<bool> GLAD_INITED{false};
		if (GLAD_INITED.load())
			return;

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
			throw std::runtime_error("GLAD init failed");
		GLAD_INITED.store(true);
	}

	bool Window::isOpen() const
	{
		return glfwWindow && !glfwWindowShouldClose(glfwWindow);
	}
	void Window::close() noexcept
	{
		if (glfwWindow)
			glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
	}

	void Window::endFrame() noexcept
	{
		if (!glfwWindow)
			return;

		glfwSwapBuffers(glfwWindow);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		metricsPrivate.onFrameRendered();
	}
	void Window::newLayer() noexcept
	{
		if (!glfwWindow)
			return;

		glClear(GL_DEPTH_BUFFER_BIT);
	}

	Window::Window(const std::string& title, const WindowPreset& preset)
		: transform(*this),
		cursor(*this),
		focus(*this),
		properties(*this),
		context(*this),
		metrics(*this),
		metricsPrivate(metrics)
	{
		initialize(title, preset);
	}

	Window::Window(const std::string& title, int width, int height)
		: transform(*this),
		cursor(*this),
		focus(*this),
		properties(*this),
		context(*this),
		metrics(*this),
		metricsPrivate(metrics)
	{
		WindowPreset preset;
		preset.window.width = width;
		preset.window.height = height;

		initialize(title, preset);
	}
}


namespace KanCore::Graphics::WindowDetails
{
	// -------------------- Transform --------------------

	void Transform::setWindowSize(const Size2Di& size)
	{
		GLFWwindow* context = window.getContext();
		glfwSetWindowSize(context, size.width, size.height);
	}
	Size2Di Transform::getWindowSize() const noexcept 
	{ 
		GLFWwindow* context = window.getContext();
		int x, y;

		glfwGetWindowSize(context, &x, &y);
		return Size2Di{static_cast<size_t>(x), static_cast<size_t>(y)};
	}
	Size2Di Transform::getFramebufferSize() const noexcept
	{
		GLFWwindow* context = window.getContext();
		int x, y;

		glfwGetFramebufferSize(context, &x, &y);
		return Size2Di{ static_cast<size_t>(x), static_cast<size_t>(y) };
	}
	float Transform::getAspectRation() const noexcept
	{
		GLFWwindow* context = window.getContext();
		int x, y;

		glfwGetFramebufferSize(context, &x, &y);
		return static_cast<float>(x) / static_cast<float>(y);
	}

	// -------------------- Cursor --------------------
	void Cursor::enable()
	{
		GLFWwindow* context = window.getContext();
		glfwSetInputMode(context, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		_isVisible = true;
	}

	void Cursor::hide()
	{
		GLFWwindow* context = window.getContext();
		glfwSetInputMode(context, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		_isVisible = false;
	}

	void Cursor::disable()
	{
		GLFWwindow* context = window.getContext();
		glfwSetInputMode(context, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		_isVisible = false;
	}
	bool Cursor::isVisible() const noexcept
	{
		return _isVisible;
	}

	void Cursor::setPosition(const Vec2i& position)
	{
		GLFWwindow* context = window.getContext();
		glfwSetCursorPos(context, static_cast<double>(position.x), static_cast<double>(position.y));
	}
	Vec2i Cursor::getPosition() const noexcept
	{
		GLFWwindow* context = window.getContext();
		double x, y;
		glfwGetCursorPos(context, &x, &y);
		return Vec2i{ static_cast<int>(x), static_cast<int>(y) };
	}

	// -------------------- Focus --------------------

	void Focus::requestAttention() const noexcept
	{
		GLFWwindow* context = window.getContext();
		glfwRequestWindowAttention(context);
	}
	bool Focus::hasFocus() const noexcept
	{
		GLFWwindow* context = window.getContext();
		return glfwGetWindowAttrib(context, GLFW_FOCUSED) != 0;
	}

	void Focus::gainFocus() const noexcept
	{
		GLFWwindow* context = window.getContext();
		glfwFocusWindow(context);
	}
	// -------------------- Metrics --------------------
	Metrics::Metrics(IWindowPrivate& window)
		: window(window)
	{
		auto now = std::chrono::steady_clock::now();
		_lastFpsCheck = now;
		_lastFrameTime = now;

		_framesCount = 0;
		_fps = 0;
		 _deltaTimeSeconds = 0;
	}
	void Metrics::onFrameRendered()
	{
		using namespace std::chrono;
		auto now = steady_clock::now();

		auto delta = now - _lastFrameTime;
		_deltaTimeSeconds = duration<double>(delta).count();
		_lastFrameTime = now;
		_framesCount++;

		//FPS 
		auto fpsCheckDuration = duration<double>(now - _lastFpsCheck).count();
		if (fpsCheckDuration >= 1.0) 
		{
			_fps = static_cast<float>(_framesCount) / static_cast<float>(fpsCheckDuration);
			
			_lastFpsCheck = now;
			_framesCount = 0;
		}
	}

	float Metrics::getFPS() const noexcept
	{ 
		return _fps;
	}
	
	double Metrics::getDeltaTimeSeconds() const noexcept
	{
		return _deltaTimeSeconds;
	}

	// -------------------- Properties --------------------

	void Properties::setTitle(const std::string& title)
	{
		_title = title;
		GLFWwindow* context = window.getContext();
		glfwSetWindowTitle(context, _title.c_str());
	}

	void Properties::setVisible(bool flag)
	{
		GLFWwindow* context = window.getContext();
		if (flag)
			glfwShowWindow(context);
		else
			glfwHideWindow(context);
	}

	void Properties::setResizable(bool flag)
	{
		GLFWwindow* context = window.getContext();
		glfwSetWindowAttrib(context, GLFW_RESIZABLE, flag ? GLFW_TRUE : GLFW_FALSE);
	}

	void Properties::setDecorated(bool flag)
	{
		GLFWwindow* context = window.getContext();
		glfwSetWindowAttrib(context, GLFW_DECORATED, flag ? GLFW_TRUE : GLFW_FALSE);
	}

	// -------------------- Context --------------------

	void Context::makeCurrent() noexcept
	{
		GLFWwindow* context = window.getContext();
		glfwMakeContextCurrent(context);
	}

	void Context::setVSync(bool enabled) noexcept
	{
		GLFWwindow* context = window.getContext();
		glfwMakeContextCurrent(context);
		glfwSwapInterval(enabled ? 1 : 0);
	}
	void Context::setViewport(float x, float y, float width, float height) noexcept
	{
		GLFWwindow* context = window.getContext();
		glfwMakeContextCurrent(context);
		glViewport(x, y, width, height);
	}



}