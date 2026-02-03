#include <iostream>
#include "include/OpenGL/GLVertexArray.hpp"
#include "include/OpenGL/GLShader.hpp"

#include "include/Window.hpp"
#include "include/Events.hpp"
#include "include/Audio.hpp"


#include <fstream>
std::string readFile(const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open())
        throw std::runtime_error("failed to open file: " + path);

    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    return content;   
}


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
    Audio::SoundBufferPtr buffer = Audio::loadFromFile("resources/Sounds12.mp3");
    Audio::Sound sound(buffer);
   
    sound.looped(true).play();


    
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

   

    events.keyboard.addListener(0, keyboard);
    events.windowState.addListener(0, windowState);

    window.context.setVSync(false);
    window.focus.gainFocus();

    //========================

    Vec3f positions[] =
    {
        {-0.5f, -0.5f, 0.f}, 
        { 0.5f, -0.5f, 0.f}, 
        { 0.5f,  0.5f, 0.f}, 
        {-0.5f,  0.5f, 0.f}  
    };
    OpenGL::GLStaticVBO<Vec3f> vbo(positions);
    const GLuint vboLocation = 0;
    GLuint indices[] = { 0, 1, 2, 2, 3, 0 };
    OpenGL::GLStaticVBO<GLuint> ebo(indices);
    OpenGL::GLVertexArray vao;

    vao.attachVertexBuffer(vboLocation, vbo);
    vao.setAttributeFormat(vboLocation, 3, OpenGL::GLAttributeType::FLOAT, false, 0);
    vao.setAttributeBinding(vboLocation, vboLocation);
    vao.enableAttribute(vboLocation);
    vao.attachElementBuffer(ebo);

    OpenGL::GLShaderProgram program;
    
    {
        std::string frag = readFile("resources/shader.frag");
        std::string vert = readFile("resources/shader.vert");
        OpenGL::GLShader fragment(frag, OpenGL::GLShaderType::FRAGMENT);
        OpenGL::GLShader vertex(vert, OpenGL::GLShaderType::VERTEX);

        OpenGL::linkShaderProgram(program, { fragment, vertex });
    }

    program.use();
    vao.bind();
    while (window) 
    {
        events.pollEvents();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        window.endFrame();
    }
    return 0;
}
