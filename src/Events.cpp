#include "../include/Events.hpp"


namespace KanCore::Input::EventDetails
{
	// ================= Keyboard =================

	void Keyboard::notify(const KeyboardEvent& event)
	{
		for (auto& [id, listener] : listeners)
		{
			if (listener)
				listener(event);
		}
	}

	bool Keyboard::isKeyPressed(KeyboardKey key)
	{
		GLFWwindow* context = Graphics::WindowDetails::getNativeWindow(bindedWindow);
		int state = glfwGetKey(context, static_cast<int>(key));

		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	void Keyboard::addListener(KeyboardListenerID id, KeyEventListener listener)
	{
		listeners[id] = std::move(listener);
	}

	void Keyboard::removeListener(KeyboardListenerID id)
	{
		listeners.erase(id);
	}

	// ================= Mouse =================

	void Mouse::notify(const MouseEvent& event)
	{
		for (auto& [id, listener] : listeners)
		{
			if (listener)
				listener(event);
		}
	}

	bool Mouse::isButtonPressed(MouseButton button)
	{
		GLFWwindow* context = Graphics::WindowDetails::getNativeWindow(bindedWindow);
		int state = glfwGetMouseButton(context, static_cast<int>(button));

		return state == GLFW_PRESS;
	}

	void Mouse::addListener(MouseListenerID id, MouseEventListener listener)
	{
		listeners[id] = std::move(listener);
	}

	void Mouse::removeListener(MouseListenerID id)
	{
		listeners.erase(id);
	}

	// ================= TextInput =================

	void TextInput::notify(const TextInputEvent& event)
	{
		for (auto& [id, listener] : listeners)
		{
			if (listener)
				listener(event);
		}
	}

	void TextInput::addListener(TextInputListenerID id, TextInputListener listener)
	{
		listeners[id] = std::move(listener);
	}

	void TextInput::removeListener(TextInputListenerID id)
	{
		listeners.erase(id);
	}

	//=== WindowTransform ===//
	void WindowTransform::notify(const WindowTransformEvent& event)
	{
		for (auto& [id, listener] : listeners)
			listener(event);
	}

	void WindowTransform::addListener(WindowTransformListenerID id, WindowTransformListener listener)
	{
		listeners[id] = std::move(listener);
	}

	void WindowTransform::removeListener(WindowTransformListenerID id)
	{
		listeners.erase(id);
	}

	//=== WindowState ===//
	void WindowState::notify(const WindowStateEvent& event)
	{
		for (auto& [id, listener] : listeners)
			listener(event);
	}

	void WindowState::addListener(WindowStateListenerID id, WindowStateListener listener)
	{
		listeners[id] = std::move(listener);
	}

	void WindowState::removeListener(WindowStateListenerID id)
	{
		listeners.erase(id);
	}

	//=== FileDrop ===//
	void FileDrop::notify(const FileDropEvent& event)
	{
		for (auto& [id, listener] : listeners)
			listener(event);
	}

	void FileDrop::addListener(FileDropListenerID id, FileDropListener listener)
	{
		listeners[id] = std::move(listener);
	}

	void FileDrop::removeListener(FileDropListenerID id)
	{
		listeners.erase(id);
	}

	
}

namespace KanCore::Input
{
	//=== Events ===//
	Events::Events(Graphics::Window& window)
		: bindedWindow(window),
		keyboard(window),
		mouse(window)
	{
		GLFWwindow* context = Graphics::WindowDetails::getNativeWindow(bindedWindow);
		glfwSetWindowUserPointer(context, this);

		// Keyboard
		glfwSetKeyCallback(context, [](GLFWwindow* w, int key, int sc, int action, int mods)
			{
				Events* self = reinterpret_cast<Events*>(glfwGetWindowUserPointer(w));
				if (self) self->keyboardCallback(key, sc, action, mods);
			});

		// Mouse
		glfwSetMouseButtonCallback(context, [](GLFWwindow* w, int button, int action, int mods)
			{
				Events* self = reinterpret_cast<Events*>(glfwGetWindowUserPointer(w));
				if (self) self->mouseCallback(button, action, mods);
			});

		// Text input
		glfwSetCharCallback(context, [](GLFWwindow* w, unsigned int codepoint)
			{
				Events* self = reinterpret_cast<Events*>(glfwGetWindowUserPointer(w));
				if (self) self->textInputCallback(codepoint);
			});

		// Window transform
		glfwSetWindowSizeCallback(context, [](GLFWwindow* w, int width, int height)
			{
				Events* self = reinterpret_cast<Events*>(glfwGetWindowUserPointer(w));
				if (self) self->windowSizeCallback(width, height);
			});
		glfwSetWindowPosCallback(context, [](GLFWwindow* w, int xpos, int ypos)
			{
				Events* self = reinterpret_cast<Events*>(glfwGetWindowUserPointer(w));
				if (self) self->windowPosCallback(xpos, ypos);
			});

		// Window state
		glfwSetWindowCloseCallback(context, [](GLFWwindow* w)
			{
				Events* self = reinterpret_cast<Events*>(glfwGetWindowUserPointer(w));
				if (self) self->windowCloseCallback();
			});
		glfwSetWindowFocusCallback(context, [](GLFWwindow* w, int focused)
			{
				Events* self = reinterpret_cast<Events*>(glfwGetWindowUserPointer(w));
				if (self) self->windowFocusCallback(focused);
			});
		glfwSetWindowIconifyCallback(context, [](GLFWwindow* w, int iconified)
			{
				Events* self = reinterpret_cast<Events*>(glfwGetWindowUserPointer(w));
				if (self) self->windowIconifyCallback(iconified);
			});
		glfwSetWindowMaximizeCallback(context, [](GLFWwindow* w, int maximized)
			{
				Events* self = reinterpret_cast<Events*>(glfwGetWindowUserPointer(w));
				if (self) self->windowMaximizeCallback(maximized);
			});
		glfwSetFramebufferSizeCallback(context, [](GLFWwindow* w, int width, int height)
			{
				Events* self = reinterpret_cast<Events*>(glfwGetWindowUserPointer(w));
				if (self) self->framebufferSizeCallback(width, height);
			});

		// File drop
		glfwSetDropCallback(context, [](GLFWwindow* w, int count, const char** paths)
			{
				Events* self = reinterpret_cast<Events*>(glfwGetWindowUserPointer(w));
				if (self) self->dropCallback(count, paths);
			});
	}

	//=== Callbacks ===//
	void Events::keyboardCallback(int key, int scancode, int action, int mods)
	{
		KeyboardEvent event{};
		event.key = static_cast<KeyboardKey>(key);
		event.scancode = scancode;
		event.action = (action == GLFW_PRESS) ? KeyboardAction::JustPressed :
			(action == GLFW_RELEASE) ? KeyboardAction::JustReleased :
			KeyboardAction::JustReleased;

		static_cast<EventDetails::IKeyboard&>(keyboard).notify(event);
	}

	void Events::mouseCallback(int button, int action, int mods)
	{
		MouseEvent event{};
		event.button = static_cast<MouseButton>(button);
		event.action = (action == GLFW_PRESS) ? MouseAction::JustPressed
			: MouseAction::JustReleased;

		double xpos, ypos;
		GLFWwindow* context = Graphics::WindowDetails::getNativeWindow(bindedWindow);
		glfwGetCursorPos(context, &xpos, &ypos);
		event.currentCursorPos = Vec2i{ int(xpos), int(ypos) };

		static_cast<EventDetails::IMouse&>(mouse).notify(event);
	}

	void Events::textInputCallback(unsigned int codepoint)
	{
		TextInputEvent event{};
		event.codepoint = static_cast<char32_t>(codepoint);

		static_cast<EventDetails::ITextInput&>(textInput).notify(event);
	}

	void Events::windowSizeCallback(int width, int height)
	{
		WindowTransformEvent event{};
		event.size = Vec2i{ width, height };
		static_cast<EventDetails::IWindowTransform&>(windowTransform).notify(event);
	}

	void Events::windowPosCallback(int xpos, int ypos)
	{
		WindowTransformEvent event{};
		event.position = Vec2i{ xpos, ypos };
		static_cast<EventDetails::IWindowTransform&>(windowTransform).notify(event);
	}

	void Events::windowCloseCallback()
	{
		WindowStateEvent event{};
		event.state = WindowState::Closed;
		event.value = true;
		static_cast<EventDetails::IWindowState&>(windowState).notify(event);
	}

	void Events::windowFocusCallback(int focused)
	{
		WindowStateEvent event{};
		event.state = WindowState::Focused;
		event.value = focused != 0;
		static_cast<EventDetails::IWindowState&>(windowState).notify(event);
	}

	void Events::windowIconifyCallback(int iconified)
	{
		WindowStateEvent event{};
		event.state = WindowState::Minimized;
		event.value = iconified != 0;
		static_cast<EventDetails::IWindowState&>(windowState).notify(event);
	}

	void Events::windowMaximizeCallback(int maximized)
	{
		WindowStateEvent event{};
		event.state = WindowState::Maximized;
		event.value = maximized != 0;
		static_cast<EventDetails::IWindowState&>(windowState).notify(event);
	}

	void Events::framebufferSizeCallback(int width, int height)
	{
		WindowTransformEvent event{};
		event.size = Vec2i{ width, height };
		static_cast<EventDetails::IWindowTransform&>(windowTransform).notify(event);
	}

	void Events::dropCallback(int count, const char** paths)
	{
		FileDropEvent event{};
		for (int i = 0; i < count; i++)
			event.paths.emplace_back(paths[i]);
		static_cast<EventDetails::IFileDrop&>(fileDrop).notify(event);
	}
}
