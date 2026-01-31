#include <iostream>
#include "include/Window.hpp"
#include "include/Events.hpp"

int main()
{
    using namespace KanCore;

    Graphics::WindowPreset preset;
    preset.window.resizable = false;
    preset.window.width = 1200;
    preset.window.height = 900;

    //'preset' is not required
    Graphics::Window window("Engine test", preset);
    Input::Events events(window);

    Input::KeyEventListener keyboard = [&window, &events](Input::KeyboardEvent event)
        {
            if (event.action == Input::KeyboardAction::JustPressed)
            {
                if (event.key == Input::KeyboardKey::F && events.keyboard.isKeyPressed(Input::KeyboardKey::LeftControl))
                    std::cout << "\rFPS: " << window.metrics.getFPS();
            }
        };

    Input::FileDropListener fileDrop = [](Input::FileDropEvent event) 
        {
            std::cout << "File drop event:\n";
            for (auto& path : event.paths)
            {
                std::cout << path << std::endl;
            }
        };

    Input::WindowStateListener windowState = [&window](Input::WindowStateEvent event)
        {
            if (event.state == Input::WindowState::Closed)
            {
                std::cout << "Window closed\n";
            }
            else if (event.state == Input::WindowState::Focused)
            {
                if (event.value)
                    std::cout << "Window has focus\n";
                else
                {
                    std::cout << "Window lose focus\n";
                    window.focus.requestAttention();
                }
            }
        };


    window.context.setVSync(false);

    events.keyboard.addListener(0, keyboard);
    events.fileDrop.addListener(0, fileDrop);
    events.windowState.addListener(0, windowState);
    while (window) 
    {
        events.pollEvents();
        window.endFrame();
    }
    return 0;
}
