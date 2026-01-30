#include "../include/Window.hpp"

namespace KanCore::Graphics
{

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

	void Transform::setSize(const Size2D& size)
	{
		GLFWwindow* context = window.getContext();
		glfwSetWindowSize(context, size.width, size.height);
	}
	Size2D Transform::getSize() const noexcept 
	{ 
		GLFWwindow* context = window.getContext();
		int x, y;

		glfwGetWindowSize(context, &x, &y);
		return Size2D{static_cast<float>(x), static_cast<float>(y)};
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
	void Metrics::onFrameRendered()
	{
		_framesCount++;
	}

	float Metrics::getFPS() noexcept
	{ 
		using namespace std::chrono; 
		steady_clock::time_point currentTime = steady_clock::now();
		float duration = duration_cast<seconds>(currentTime - _lastFpsCheck).count(); 
		float fps = static_cast<float>(_framesCount) / duration; 
		_framesCount = 0; 
		_lastFpsCheck = currentTime;
		return fps;
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



}