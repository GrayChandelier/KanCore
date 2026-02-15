#include <iostream>
#include "include/OpenGL/GLVertexArray.hpp"
#include "include/OpenGL/GLShader.hpp"
#include "include/OpenGL/GLTextureSampler.hpp"
#include "include/OpenGL/GLTexture.hpp"
#include "include/OpenGL/GLFramebuffer.hpp"
#include "include/Window.hpp"
#include "include/Events.hpp"
#include "include/Audio.hpp"
#include "include/Camera.hpp"
#include "include/Image.hpp"
#include "include/Text.hpp"


#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <algorithm> // std::clamp
#include <unordered_set>

#include "include/Threads.hpp"
#include "include/Scheduler.hpp"
#include "include/Stopwatch.hpp"
#include "include/TextureAtlas.hpp"

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
    preset.window.width = 2000;
    preset.window.height = 1600;
    preset.graphics.glMajorVersion = 5;
    preset.graphics.glMinorVersion = 5;

 

    Graphics::Window window("Engine test", preset);
    Input::Events events(window);
    Graphics::Camera camera({ 0, 0, 0.7f }, { 0, 0, -1 });
    Audio::AudioContext audio;
    Audio::SoundBufferPtr buffer = Audio::loadFromFile("resources/Sounds12.mp3");
    Audio::Sound sound(buffer);


    sound.looped(true).play();

    const std::unordered_set<Graphics::unicode_char> whitelistCharacters = {
    0x1F602, 0x2764, 0x1F60D, 0x1F622, 0x1F60E, 0x1F44D, 0x1F44E
    };

    Graphics::CharacterFilter unicodeFilter = [&whitelistCharacters](Graphics::unicode_char candidate) -> bool
        {
            return candidate <= 255 || whitelistCharacters.contains(candidate);
        };

    Graphics::FontPtr fontPtr = Graphics::FontLoaders::loadFromFile("resources/segoe-ui-emoji_0.ttf", 64, false, unicodeFilter);


    // Слушатели событий
    Input::KeyEventListener keyboard = [&window, &events, &sound](Input::KeyboardEvent event)
        {
            static size_t tex = 0;
            if (event.action == Input::KeyboardAction::JustPressed)
            {
                if (event.key == Input::KeyboardKey::F && events.keyboard.isKeyPressed(Input::KeyboardKey::LeftControl))
                    std::cout << "\rFPS: " << window.metrics.getFPS() << "\n";
                if (event.key == Input::KeyboardKey::Home)
                {
                    std::cout << "Play sound\n";
                    bool flag = sound.isPaused();
                    if (flag) sound.resume();
                    else sound.pause();
                }

            }
        };

    Input::MouseEventListener mouse = [&camera](Input::MouseEvent event)
        {
            if (event.action != Input::MouseAction::Moved) return;


            constexpr float sensitivity = 0.18f;  
            constexpr float maxPitchDeg = 89.0f;  
            Vec2f delta = event.cursorPosDelta;

        
        };




    Input::WindowStateListener windowState = [&window, &sound](Input::WindowStateEvent event)
        {
            if (event.state == Input::WindowState::Closed)
                std::cout << "Window closed\n";
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

    Graphics::FormattedText text;
    text.attachFont(0, fontPtr);

    Graphics::UString str = UR"(KanCore Framework

Emoji: 😂😍😢😎👍👎❤

§1Color code: \§1 
§2Color code: \§2
§3Color code: \§3
§4Color code: \§4
§5Color code: \§5
§6Color code: \§6
§7Color code: \§7
§8Color code: \§8
§9Color code: \§9
)";

    uint16_t textSize = 128;
    text.setText(str, textSize, Vec2f(0, 100.f));



    Input::WindowTransformListener windowTransform = [&camera, &window](Input::WindowTransformEvent event)
        {
            if (event.transformation == Input::WindowTransformation::FramebufferResized)
            {
                int width = event.framebufferSize.width;
                int height = event.framebufferSize.height;
                window.context.setViewport(0, 0, width, height);
                camera.projection.setAspectRatio(window.transform.getAspectRation());
            }
        };



    events.keyboard.addListener(0, keyboard);
    events.mouse.addListener(0, mouse);
    events.windowState.addListener(0, windowState);
    events.windowTransform.addListener(0, windowTransform);

    camera.projection.setAspectRatio(window.transform.getAspectRation());
    window.context.setVSync(false);
    window.focus.gainFocus();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    
    // Текстура и объект
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

    // VAO для квадрата
    struct Vertex { Vec3f position; Vec2f texCoord; };

    Vertex vertices[] = {
        {{-0.5f, -0.5f, 0.f}, {0.f, 1.f}},   // левый нижний
        {{ 0.5f, -0.5f, 0.f}, {1.f, 1.f}},   // правый нижний
        {{ 0.5f,  0.5f, 0.f}, {1.f, 0.f}},   // правый верхний
        {{-0.5f,  0.5f, 0.f}, {0.f, 0.f}}    // левый верхний
    };


    GLuint indices[] = { 0, 1, 2,   2, 3, 0 };

    OpenGL::GLStaticVBO<Vertex> vbo(vertices);
    OpenGL::GLStaticVBO<GLuint> ebo(indices);
    OpenGL::GLVertexArray vao;
    vao.attachVertexBuffer(0, vbo);
    vao.setAttributeFormat(0, 3, OpenGL::GLAttributeType::FLOAT, false, offsetof(Vertex, position));
    vao.setAttributeBinding(0, 0);
    vao.enableAttribute(0);
    vao.setAttributeFormat(1, 2, OpenGL::GLAttributeType::FLOAT, false, offsetof(Vertex, texCoord));
    vao.setAttributeBinding(1, 0);
    vao.enableAttribute(1);
    vao.attachElementBuffer(ebo);

    
    // Шейдер основной сцены
    OpenGL::GLShaderProgram program;
    try
    {
        std::string vert = readFile("resources/shader.vert");
        std::string frag = readFile("resources/shader.frag");
        OpenGL::GLShader vertex(vert, OpenGL::GLShaderType::VERTEX);
        OpenGL::GLShader fragment(frag, OpenGL::GLShaderType::FRAGMENT);
        OpenGL::linkShaderProgram(program, { vertex, fragment });
    }
    catch (const OpenGL::GLShaderException& ex)
    {
        std::cerr << "Failed to compile shader program: " << ex.what() << "\n";
        return 1;
    }

    // Камерные uniform-локации
    Graphics::CameraUniforms camUniforms;
    camUniforms.projectionUniformLocation = 0;
    camUniforms.viewUniformLocation = 1;

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));

    Utils::Stopwatch stopwatch;

    constexpr float cameraSpeed = 1.f;


    glClearColor(0.7f, 0.9f, 1.f, 1.f);


    Utils::Scheduler scheduler;

    std::function<void()> nextTask;

    auto task = [&nextTask, &scheduler, &camera]()
        {
            camera.controller.rotate(Graphics::CameraRotationAxis::AbsoluteY, 10);
            scheduler.schedule(std::chrono::milliseconds(50), nextTask);
        };
    nextTask = task;

    scheduler.schedule(std::chrono::seconds(1), nextTask);

    while (window)
    {
        const float dt = window.metrics.getDeltaTimeSeconds();

       
        if (events.keyboard.isKeyPressed(Input::KeyboardKey::W))
        {
            camera.controller.move({ 0, 0, cameraSpeed * window.metrics.getDeltaTimeSeconds() });
        }
        else if (events.keyboard.isKeyPressed(Input::KeyboardKey::S))
        {
            camera.controller.move({ 0, 0, -cameraSpeed * window.metrics.getDeltaTimeSeconds() });
        }


        program.use();
        camera.apply(camUniforms);
        glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(model));
        texture.bind(0);


        sampler.bind(0);
        vao.bind();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);




        text.draw(window.transform.getFramebufferSize().width, window.transform.getFramebufferSize().height);
        events.pollEvents();   
        window.endFrame();

    }

    return 0;
}