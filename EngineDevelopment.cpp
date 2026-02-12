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
#include "include/Font.hpp"

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

    const std::unordered_set<Graphics::unicode_char> emojiWhitelist = {
    0x1F602, 0x2764, 0x1F60D, 0x1F622, 0x1F60E, 0x1F44D, 0x1F44E
    };

    Graphics::CharacterFilter unicodeFilter = [&emojiWhitelist](Graphics::unicode_char candidate) -> bool
        {
            if (candidate <= 255 || emojiWhitelist.contains(candidate))
                return true;
            else
                return false;
        };

    Graphics::FontPtr fontPtr = Graphics::FontLoaders::loadFromFile("resources/segoe-ui-emoji_0.ttf", 96, false, unicodeFilter);


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

    // Переменные для эффектов
    float blurStrength = 0.0f;   // 0..10
    float sharpness = 1.0f;      // 0..30

    Input::WindowTransformListener windowTransform = [&camera, &window, &blurStrength, &sharpness](Input::WindowTransformEvent event)
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


    // Fullscreen quad
    struct FullscreenQuad {
        OpenGL::GLVertexArray vao;
        struct Vertex { Vec2f pos; Vec2f uv; };
        FullscreenQuad() {
            Vertex verts[4] = {
                {{-1,-1}, {0,0}},
                {{ 1,-1}, {1,0}},
                {{ 1, 1}, {1,1}},
                {{-1, 1}, {0,1}}
            };
            OpenGL::GLStaticVBO<Vertex> vbo(verts);
            vao.attachVertexBuffer(0, vbo);
            vao.setAttributeFormat(0, 2, OpenGL::GLAttributeType::FLOAT, false, offsetof(Vertex, pos));
            vao.setAttributeBinding(0, 0);
            vao.enableAttribute(0);
            vao.setAttributeFormat(1, 2, OpenGL::GLAttributeType::FLOAT, false, offsetof(Vertex, uv));
            vao.setAttributeBinding(1, 0);
            vao.enableAttribute(1);
        }
        void draw() const 
        {
            vao.bind();
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }
    } fsQuad;

    // Текстуры и framebuffer'ы для пост-обработки
    auto fbSize = window.transform.getFramebufferSize();
    OpenGL::GLTexture2D sceneTexture(fbSize.width, fbSize.height, OpenGL::TextureInternalFormat::RGBA8);
    OpenGL::GLTexture2D blurTexture(fbSize.width, fbSize.height, OpenGL::TextureInternalFormat::RGBA8);

    OpenGL::GLFramebuffer fbScene;
    fbScene.attach(OpenGL::FramebufferAttachment::Color0, sceneTexture);

    OpenGL::GLFramebuffer fbBlur;
    fbBlur.attach(OpenGL::FramebufferAttachment::Color0, blurTexture);

    // Шейдеры пост-обработки
    OpenGL::GLShaderProgram postProgram;
    try
    {
        std::string vert = readFile("resources/postprocessing.vert");
        std::string blurFrag = readFile("resources/blur.frag");
        OpenGL::GLShader v(vert, OpenGL::GLShaderType::VERTEX);
        OpenGL::GLShader f(blurFrag, OpenGL::GLShaderType::FRAGMENT);
        OpenGL::linkShaderProgram(postProgram, { v, f });
    }
    catch (const OpenGL::GLShaderException& ex)
    {
        std::cerr << "Failed to compile post-processing program: " << ex.what() << "\n";
        return 1;
    }

    OpenGL::GLShaderProgram sharpenProgram;
    try
    {
        std::string vert = readFile("resources/postprocessing.vert");
        std::string frag = readFile("resources/sharpen.frag");
        OpenGL::GLShader v(vert, OpenGL::GLShaderType::VERTEX);
        OpenGL::GLShader f(frag, OpenGL::GLShaderType::FRAGMENT);
        OpenGL::linkShaderProgram(sharpenProgram, { v, f });
    }
    catch (const OpenGL::GLShaderException& ex)
    {
        std::cerr << "Failed to compile sharpen program: " << ex.what() << "\n";
        return 1;
    }

    // Камерные uniform-локации
    Graphics::CameraUniforms camUniforms;
    camUniforms.projectionUniformLocation = 0;
    camUniforms.viewUniformLocation = 1;

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));

    Utils::Stopwatch stopwatch;
    while (window)
    {
        float dt = window.metrics.getDeltaTimeSeconds();

        // Управление эффектами
        if (events.keyboard.isKeyPressed(Input::KeyboardKey::X)) blurStrength += 3.0f * dt;
        if (events.keyboard.isKeyPressed(Input::KeyboardKey::Z)) blurStrength -= 3.0f * dt;
        blurStrength = std::clamp(blurStrength, 0.0f, 10.0f);

        if (events.keyboard.isKeyPressed(Input::KeyboardKey::E)) sharpness += 10.0f * dt;
        if (events.keyboard.isKeyPressed(Input::KeyboardKey::Q)) sharpness -= 10.0f * dt;
        sharpness = std::clamp(sharpness, 0.0f, 30.0f);

        // 1. Рендерим сцену в fbScene
        fbScene.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        program.use();
        camera.apply(camUniforms);
        glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(model));
        //texture.bind(0);
        fontPtr->use(0);

        sampler.bind(0);
        vao.bind();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        // 2. Горизонтальное размытие → blurTexture
        fbBlur.bind();
        glClear(GL_COLOR_BUFFER_BIT);
        postProgram.use();
        postProgram.setUniform2f("uDirection", 1.0f, 0.0f);
        postProgram.setUniform2f("uResolution", static_cast<float>(fbSize.width), static_cast<float>(fbSize.height));
        postProgram.setUniform1f("uStrength", blurStrength);
        sceneTexture.bind(0);
        fsQuad.draw();

        // 3. Вертикальное размытие → обратно в sceneTexture
        fbScene.bind();
        glClear(GL_COLOR_BUFFER_BIT);
        postProgram.setUniform2f("uDirection", 0.0f, 1.0f);
        blurTexture.bind(0);
        fsQuad.draw();

        // 4. Применяем sharpen и выводим на экран
        OpenGL::GLFramebuffer::bindDefault();
        glClear(GL_COLOR_BUFFER_BIT);

        sharpenProgram.use();
        sharpenProgram.setUniform2f("uResolution",
            static_cast<float>(fbSize.width),
            static_cast<float>(fbSize.height));

        float sharpnessModifier = 10+ (stopwatch.elapsedSinceStartMs().count()/50 %4)*10;
        sharpenProgram.setUniform1f("uSharpness", sharpness);

        sharpenProgram.setUniform1f("uTime", dt);

        sceneTexture.bind(0);
        fsQuad.draw();

        events.pollEvents();
        window.endFrame();
    }

    return 0;
}