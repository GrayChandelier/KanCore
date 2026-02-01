#include <iostream>
#include "include/Window.hpp"
#include "include/Events.hpp"

#include "include/Audio.hpp"

#include <filesystem>

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

    Audio::AudioContext audio;
    Audio::SoundBufferPtr buffer = Audio::loadFromFile("clarinet-46466.mp3");
    Audio::Sound sound(buffer);

    sound.looped(true).setPitch(0.5f).play();
 
    Input::KeyEventListener keyboard = [&window, &events, &sound, &audio](Input::KeyboardEvent event)
        {
            if (event.action == Input::KeyboardAction::JustPressed)
            {
                if (event.key == Input::KeyboardKey::F && events.keyboard.isKeyPressed(Input::KeyboardKey::LeftControl))
                    std::cout << "\rFPS: " << window.metrics.getFPS()<<"\n";

                if (event.key == Input::KeyboardKey::S)
                {
                    std::cout << "Play sound\n";
                    bool flag = sound.isPaused();

                    if (flag)
                        sound.resume();
                    else
                        sound.pause();
                }

                else if (event.key == Input::KeyboardKey::D)
                {
                    std::cout << "Listener moved\n";
                   sound.setPosition({ 0, 0, 10 });
                }
               
            }
        };

    std::vector<std::shared_ptr<Audio::Sound>> sounds;
    Input::FileDropListener fileDrop = [&audio, &sounds](Input::FileDropEvent event) 
        {
            std::cout << "File drop event:\n";
            for (auto& path : event.paths)
            {
                std::filesystem::path p(path);
                if (p.extension() != ".wav")
                    continue;

                std::cout << "Play " << p.filename().string() << "\n";
                Audio::SoundBufferPtr buffer = Audio::loadFromFile(path);
                audio.sounds.saveBufferAs(p.filename().string(), buffer);
                std::shared_ptr<Audio::Sound> sound = std::make_shared<Audio::Sound>(buffer);
                sounds.push_back(sound);
                sound->play();
            }
        };

    Input::WindowStateListener windowState = [&window, &sound](Input::WindowStateEvent event)
        {
            if (event.state == Input::WindowState::Closed)
            {
                std::cout << "Window closed\n";
            }
            else if (event.state == Input::WindowState::Focused)
            {
                if (event.value)
                {
                    std::cout << "Window has focus\n";
                    sound.resume();
                }
                else
                {
                    std::cout << "Window lose focus\n";
                    sound.pause();
                }
            }
        };

    window.context.setVSync(false);

    events.keyboard.addListener(0, keyboard);
    events.fileDrop.addListener(0, fileDrop);
    events.windowState.addListener(0, windowState);

    window.focus.gainFocus();

    while (window) 
    {
        events.pollEvents();
        window.endFrame();
    }
    return 0;
}
