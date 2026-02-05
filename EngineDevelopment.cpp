#include <iostream>


#include "include/OpenGL/GLVertexArray.hpp"
#include "include/OpenGL/GLShader.hpp"
#include "include/OpenGL/GLTextureSampler.hpp"
#include "include/OpenGL/GLTexture.hpp"

#include "include/Window.hpp"
#include "include/Events.hpp"
#include "include/Audio.hpp"
#include "include/Camera.hpp"

#include "include/Image.hpp"

#include <glm/gtc/type_ptr.hpp>

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
    preset.window.width = 1200;
    preset.window.height = 900;
    preset.graphics.glMajorVersion = 4;
    preset.graphics.glMinorVersion = 5;

    //'preset' is not required
    Graphics::Window window("Engine test", preset);
    Input::Events events(window);

    Graphics::Camera camera({ 0,0,3 }, {0,0,-1});

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

                if (event.key == Input::KeyboardKey::Pause)
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

    Input::WindowStateListener windowState = [&window, &sound, &camera](Input::WindowStateEvent event)
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

   
    Input::WindowTransformListener windowTransform = [&camera, &window](Input::WindowTransformEvent event)
        {
            if (event.transformation == Input::WindowTransformation::FramebufferResized)
            {
                const int width = event.framebufferSize.width;
                const int height = event.framebufferSize.height;

                window.context.setViewport(0, 0, width, height);
                camera.projection.setAspectRatio(window.transform.getAspectRation());  
            }

        };

    events.keyboard.addListener(0, keyboard);
    events.windowState.addListener(0, windowState);
    events.windowTransform.addListener(0, windowTransform);

    {
        Size2Di fsize = window.transform.getFramebufferSize();

    }
    camera.projection.setAspectRatio(window.transform.getAspectRation());
    window.context.setVSync(false);
    window.focus.gainFocus();

    //========================
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Graphics::Image image;
    image.loadFromFile("resources/zombie_skull.png");

    OpenGL::GLTextureSampler sampler;
    sampler.setFilter(GL_NEAREST, GL_NEAREST);

    OpenGL::GLTexture2D texture(
         image.getSize().width,
         image.getSize().height,
         OpenGL::TextureInternalFormat::RGBA8
    );
    

    texture.setData(0, OpenGL::TextureDataFormat::RGBA, OpenGL::TextureDataType::UBYTE, image.getData().data());
    
   
    struct Vertex
    {
        Vec3f position;
        Vec2f texCoord;
    };

    Vertex vertices[] =
    {
        {{-0.5f, -0.5f, 0.f}, {0.f, 0.f}}, // нижний левый
        {{ 0.5f, -0.5f, 0.f}, {1.f, 0.f}}, // нижний правый
        {{ 0.5f,  0.5f, 0.f}, {1.f, 1.f}}, // верхний правый
        {{-0.5f,  0.5f, 0.f}, {0.f, 1.f}}  // верхний левый
    };

    GLuint indices[] = { 0,1,2, 2,3,0 };

    // Создаём VBO и EBO
    OpenGL::GLStaticVBO<Vertex> vbo(vertices);
    OpenGL::GLStaticVBO<GLuint> ebo(indices);

    OpenGL::GLVertexArray vao;
    vao.attachVertexBuffer(0, vbo);

    // позиция
    vao.setAttributeFormat(0, 3, OpenGL::GLAttributeType::FLOAT, false, offsetof(Vertex, position));
    vao.setAttributeBinding(0, 0);
    vao.enableAttribute(0);

    // текстура
    vao.setAttributeFormat(1, 2, OpenGL::GLAttributeType::FLOAT, false, offsetof(Vertex, texCoord));
    vao.setAttributeBinding(1, 0);
    vao.enableAttribute(1);

    vao.attachElementBuffer(ebo);

    OpenGL::GLShaderProgram program;
    try
    {
        std::string frag = readFile("resources/shader.frag");
        std::string vert = readFile("resources/shader.vert");
        OpenGL::GLShader fragment(frag, OpenGL::GLShaderType::FRAGMENT);
        OpenGL::GLShader vertex(vert, OpenGL::GLShaderType::VERTEX);

        OpenGL::linkShaderProgram(program, { fragment, vertex });
    } 
    catch (OpenGL::GLShaderException& ex)
    {
        std::cerr << "Failed to compile shader program: " << ex.what() << "\n";
    }

    program.use();
    program.setUniform1i("uTexture", 0);

    texture.bind(0);
    sampler.bind(0);

    vao.bind();

    Graphics::CameraUniforms camUniforms;
    camUniforms.projectionUniformLocation = 0;
    camUniforms.viewUniformLocation = 1;

    glm::mat4 model = glm::mat4(1.0f);
   // model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));

    float cameraSpeed = 10.f;

    
    while (window) 
    {
        const float deltaTime = window.metrics.getDeltaTimeSeconds();

 

        camera.apply(camUniforms);

        events.pollEvents();
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        window.endFrame();
    }
    return 0;
}
