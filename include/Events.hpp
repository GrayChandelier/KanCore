#pragma once
#include <functional>
#include "Window.hpp"
#include "InputDefines.hpp"

namespace KanCore::Input
{
	namespace EventDetails
	{
		using KeyboardListenerID = uint16_t;
		using MouseListenerID = uint16_t;
		using TextInputListenerID = uint16_t;

		using WindowTransformListenerID = uint16_t;
		//'close', 'framebuffer' and another
		using WindowStateListenerID = uint16_t;
		using FileDropListenerID = uint16_t;

		class IKeyboard
		{
		public:
			virtual void notify(const KeyboardEvent& event) = 0;
			~IKeyboard() = default;
		};

		class IMouse
		{
		public:
			virtual void notify(const MouseEvent& event) = 0;
			~IMouse() = default;
		};

		class ITextInput
		{
		public:
			virtual void notify(const TextInputEvent& event) = 0;
			~ITextInput() = default;
		};

		class IWindowTransform
		{
		public:
			virtual void notify(const WindowTransformEvent& event) = 0;
			~IWindowTransform() = default;
		};

		class IWindowState
		{
		public:
			virtual void notify(const WindowStateEvent& event) = 0;
			~IWindowState() = default;
		};

		class IFileDrop
		{
		public:
			virtual void notify(const FileDropEvent& event) = 0;
			~IFileDrop() = default;
		};

		class Keyboard : public IKeyboard
		{
		private:
			Graphics::Window& bindedWindow;

			std::unordered_map<EventDetails::KeyboardListenerID, Input::KeyEventListener> listeners;
			void notify(const KeyboardEvent& event) override;
		public:
			Keyboard(Graphics::Window& bindedWindow) : bindedWindow(bindedWindow) {}
			//permanent action
			bool isKeyPressed(KeyboardKey key);

			void addListener(KeyboardListenerID id, KeyEventListener listener);
			void removeListener(KeyboardListenerID id);
		};


		class Mouse : public IMouse
		{
		private:
			Graphics::Window& bindedWindow;

			std::unordered_map<EventDetails::MouseListenerID, Input::MouseEventListener> listeners;
			void notify(const MouseEvent& event) override;
		public:
			Mouse(Graphics::Window& bindedWindow) : bindedWindow(bindedWindow) {}

			//permanent action
			bool isButtonPressed(MouseButton button);

			void addListener(MouseListenerID id, MouseEventListener listener);
			void removeListener(MouseListenerID id);
		};

		class TextInput : public ITextInput
		{
		private:
			std::unordered_map<EventDetails::TextInputListenerID, Input::TextInputListener> listeners;
			void notify(const TextInputEvent& event) override;
		public:
			void addListener(TextInputListenerID id, TextInputListener listener);
			void removeListener(TextInputListenerID id);
		};

		class WindowTransform : public IWindowTransform
		{
		private:
			std::unordered_map<WindowTransformListenerID, WindowTransformListener> listeners;
			void notify(const WindowTransformEvent& event) override;
		public:
			WindowTransform() {}
			void addListener(WindowTransformListenerID id, WindowTransformListener listener);
			void removeListener(WindowTransformListenerID id);
		};

		class WindowState : public IWindowState
		{
		private:
			std::unordered_map<WindowStateListenerID, WindowStateListener> listeners;
			void notify(const WindowStateEvent& event) override;
		public:
			WindowState() {}
			void addListener(WindowStateListenerID id, WindowStateListener listener);
			void removeListener(WindowStateListenerID id);
		};

		class FileDrop : public IFileDrop
		{
		private:
			std::unordered_map<FileDropListenerID, FileDropListener> listeners;
			void notify(const FileDropEvent& event) override;
		public:
			FileDrop() {}
			void addListener(FileDropListenerID id, FileDropListener listener);
			void removeListener(FileDropListenerID id);
		};
		
	}
	class Events 
	{
	private:
		Graphics::Window& bindedWindow;

		void keyboardCallback(int key, int scancode, int action, int mods);
		void mouseCallback(int button, int action, int mods);
		void textInputCallback(unsigned int codepoint);
		void windowSizeCallback(int width, int height);
		void windowPosCallback(int xpos, int ypos);
		void windowCloseCallback();
		void windowFocusCallback(int focused);
		void windowIconifyCallback(int iconified);
		void windowMaximizeCallback(int maximized);
		void framebufferSizeCallback(int width, int height);
		void dropCallback(int count, const char** paths);

	public:
		Events(Graphics::Window& window);

		EventDetails::Keyboard keyboard;
		EventDetails::Mouse mouse;
		EventDetails::TextInput textInput;

		EventDetails::WindowTransform windowTransform;
		EventDetails::WindowState windowState;
		EventDetails::FileDrop fileDrop;

		inline void pollEvents()
		{
			glfwPollEvents();
		}
		inline void waitEvents()
		{
			glfwWaitEvents();
		}
	};
}