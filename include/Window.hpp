#pragma once
#include "Details.hpp"
#include <GLFW/glfw3.h>
#include <string>
#include <chrono>

namespace KanCore::Graphics
{
    class Window; //Forward declaration for WindowDetails::getNativeWindow function

    struct WindowSettings
    {
        int width = 400;
        int height = 300;
        bool resizable = true;
        bool visible = true;
        bool decorated = true;
        bool focused = true;
        bool autoIconify = false;
        bool floating = false;
        bool maximized = false;
        bool centerCursor = false;
        bool focusOnShow = true;
    };

    struct GraphicsContextSettings
    {
        bool forwardCompat = false;
        bool debugContext = false;
        bool noErrorContext = false;
        bool stereo = false;
        bool doubleBuffer = true;
        bool vsync = true;

        int glMajorVersion = 3;
        int glMinorVersion = 3;
    };

    struct WindowPreset
    {
        WindowSettings window;
        GraphicsContextSettings graphics;

        GLFWwindow* shareWith = nullptr;
        GLFWmonitor* monitor = nullptr;
    };

    namespace WindowDetails
    {
        class GlfwContextRAII
        {
            friend void initializeLibraries();
        private:
            GlfwContextRAII()
            {
                if (!glfwInit()) throw std::runtime_error("GLFW init failed");
            }
        public:
            GlfwContextRAII(GlfwContextRAII&) = delete;
            GlfwContextRAII(GlfwContextRAII&&) = delete;
            ~GlfwContextRAII() { glfwTerminate(); }
        };
        inline void initializeLibraries()
        {
            static GlfwContextRAII context;
        }



        class IWindowPrivate
        {
        public:
            //throws if window is not initialized
            virtual GLFWwindow* getContext() = 0;
        };

        class Transform
        {
        private:
            IWindowPrivate& window;
        public:
            explicit Transform(IWindowPrivate& window) : window(window) {}
            void setSize(const Size2D& size);
            Size2D getSize() const noexcept;
        };

        class Cursor
        {
        private:
            IWindowPrivate& window;
            bool _isVisible = true;

        public:
            explicit Cursor(IWindowPrivate& window) : window(window) {}

            Vec2i getPosition() const noexcept;
            bool isVisible() const noexcept;

            void enable();
            void hide();
            void disable();

            void setPosition(const Vec2i& position);
        };

        class Focus
        {
        private:
            IWindowPrivate& window;

        public:
            explicit Focus(IWindowPrivate& window) : window(window) {}

            void requestAttention() const noexcept;
            bool hasFocus() const noexcept;
            void gainFocus() const noexcept;
        };

        class IMetricsPrivate
        {
        public:
            virtual void onFrameRendered() = 0;
        };
        class Metrics : public IMetricsPrivate
        {
        private:
            IWindowPrivate& window;
            
            void onFrameRendered() override;
            std::chrono::steady_clock::time_point _lastFpsCheck;
            std::chrono::steady_clock::time_point _lastFrameTime;

            //frames between getFPS method call
            size_t _framesCount;
            float _fps;

            //duration between two frames
            double _deltaTimeSeconds;
        public:
            explicit Metrics(IWindowPrivate& window);
            float getFPS() const noexcept;
            double getDeltaTimeSeconds() const noexcept;
        };

        class Properties
        {
        private:
            IWindowPrivate& window;
            std::string _title;

        public:
            explicit Properties(IWindowPrivate& window) : window(window) {}

            void setTitle(const std::string& title);
            const std::string& getTitle() const noexcept { return _title; }

            void setVisible(bool flag);
            void setResizable(bool flag);
            void setDecorated(bool flag);
        };

        class Context
        {
        private:
            IWindowPrivate& window;

        public:
            explicit Context(IWindowPrivate& window) : window(window) {}

            void makeCurrent() noexcept;
            void setVSync(bool enabled) noexcept;
        };


        // For internal/advanced use only: returns raw GLFWwindow pointer.
        // Do not use unless you know what you are doing.
        inline GLFWwindow* getNativeWindow(WindowDetails::IWindowPrivate& window)
        {
            return window.getContext();
        }
    }

    class Window : public WindowDetails::IWindowPrivate
    {
    private:
        GLFWwindow* glfwWindow;


        inline GLFWwindow* getContext() override
        {
            if (!glfwWindow) throw std::runtime_error("GLFW context is null!");
            return glfwWindow;
        }
        void initialize(const std::string& title, const WindowPreset& preset);

        WindowDetails::IMetricsPrivate& metricsPrivate;
    public:
        WindowDetails::Transform transform;
        WindowDetails::Cursor cursor;
        WindowDetails::Focus focus;
        WindowDetails::Properties properties;
        WindowDetails::Context context;
        WindowDetails::Metrics metrics;

        Window(const std::string& title, const WindowPreset& preset);
        Window(const std::string& title, int width, int height);

        bool isOpen() const;
        void close() noexcept;

        void endFrame() noexcept;
        void newLayer() noexcept;

        explicit operator bool() const
        {
            return isOpen();
        }

        ~Window()
        {
            if (glfwWindow)
                glfwDestroyWindow(glfwWindow);
        }
    };
}


