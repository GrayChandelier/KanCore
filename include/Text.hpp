#pragma once
#include "Font.hpp"
#include "OpenGL/GLVertexArray.hpp"
#include "OpenGL/GLShader.hpp"

#include <unordered_map>
#include <vector>
#include <stdexcept>

namespace KanCore::Graphics
{
    using UString = std::u32string;

    namespace TextDetails
    {
        struct CharacterInstance
        {
            Vec2f position;
            Vec2f size;
            Vec2f uvPos;
            Vec2f uvSize;
            Vec3f colorMask{1.f, 1.f, 1.f};
        };

        static OpenGL::GLTextureSampler& textSampler()
        {
            static OpenGL::GLTextureSampler sampler = [] {
                OpenGL::GLTextureSampler s;
                s.setFilter(GL_LINEAR, GL_LINEAR);
                return s;
                }();
            return sampler;
        }

        static float quadVertices[] =
        {
            // pos      // uv
            0.f, 0.f,   0.f, 0.f,
            1.f, 0.f,   1.f, 0.f,
            0.f, 1.f,   0.f, 1.f,
            1.f, 1.f,   1.f, 1.f
        };
    }

    class FormattedText
    {
    private:
        using FontKey = uint16_t;
        std::unordered_map<FontKey, FontPtr> fonts;

        OpenGL::GLVertexArray VAO;
        OpenGL::GLStaticVBO<float> quadVBO;
        OpenGL::GLDynamicVBO<TextDetails::CharacterInstance> instanceVBO;

        std::vector<TextDetails::CharacterInstance> instances;

        OpenGL::GLShaderProgram shader;

    public:
        FormattedText()
            : quadVBO(TextDetails::quadVertices, OpenGL::GLStorageFlags::NONE),
            instanceVBO(OpenGL::GLUsage::DYNAMIC_DRAW)
        {
            VAO.bind();

            constexpr GLsizei quadStride = sizeof(float) * 4;

            // --- Quad position ---
            VAO.attachVertexBuffer(0, quadVBO, 0, quadStride);
            VAO.setAttributeFormat(0, 2, OpenGL::GLAttributeType::FLOAT, GL_FALSE, 0);
            VAO.setAttributeBinding(0, 0);
            VAO.enableAttribute(0);

            // --- Quad UV ---
            VAO.setAttributeFormat(1, 2, OpenGL::GLAttributeType::FLOAT, GL_FALSE, sizeof(float) * 2);
            VAO.setAttributeBinding(1, 0);
            VAO.enableAttribute(1);

            // --- Instance buffer ---
            VAO.attachVertexBuffer(2, instanceVBO, 0, sizeof(TextDetails::CharacterInstance));

            VAO.setAttributeFormat(2, 2, OpenGL::GLAttributeType::FLOAT, GL_FALSE,
                offsetof(TextDetails::CharacterInstance, position));
            VAO.setAttributeBinding(2, 2);
            VAO.enableAttribute(2);
            VAO.setBindingDivisor(2, 1);

            VAO.setAttributeFormat(3, 2, OpenGL::GLAttributeType::FLOAT, GL_FALSE,
                offsetof(TextDetails::CharacterInstance, size));
            VAO.setAttributeBinding(3, 2);
            VAO.enableAttribute(3);
            VAO.setBindingDivisor(3, 1);

            VAO.setAttributeFormat(4, 2, OpenGL::GLAttributeType::FLOAT, GL_FALSE,
                offsetof(TextDetails::CharacterInstance, uvPos));
            VAO.setAttributeBinding(4, 2);
            VAO.enableAttribute(4);
            VAO.setBindingDivisor(4, 1);

            VAO.setAttributeFormat(5, 2, OpenGL::GLAttributeType::FLOAT, GL_FALSE,
                offsetof(TextDetails::CharacterInstance, uvSize));
            VAO.setAttributeBinding(5, 2);
            VAO.enableAttribute(5);
            VAO.setBindingDivisor(5, 1);

            VAO.setAttributeFormat(6, 3, OpenGL::GLAttributeType::FLOAT, GL_FALSE,
                offsetof(TextDetails::CharacterInstance, colorMask));
            VAO.setAttributeBinding(6, 2);
            VAO.enableAttribute(6);
            VAO.setBindingDivisor(6, 1);

            VAO.unbind();


            OpenGL::GLShader vert(R"(
#version 460 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;

layout(location = 2) in vec2 instPos;
layout(location = 3) in vec2 instSize;
layout(location = 4) in vec2 instUVPos;
layout(location = 5) in vec2 instUVSize;
layout(location = 6) in vec3 colorMask;
uniform vec2 uScreenSize;

out vec2 fragUV;
out vec3 mask;
void main()
{
    vec2 worldPos = instPos + inPos * instSize;

    vec2 ndc = (worldPos / uScreenSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;

    gl_Position = vec4(ndc, 0.0, 1.0);

    fragUV = instUVPos + inUV * instUVSize;
    mask = colorMask;
}
            )", OpenGL::GLShaderType::VERTEX);

            OpenGL::GLShader frag(R"(
#version 460 core

in vec2 fragUV;
in vec3 mask;
uniform sampler2D fontAtlas;

out vec4 FragColor;

void main()
{
    
    FragColor = texture(fontAtlas, fragUV) * vec4(mask, 1);
}

            )", OpenGL::GLShaderType::FRAGMENT);

            OpenGL::linkShaderProgram(shader, { vert, frag });
        }

        void attachFont(FontKey key, FontPtr font)
        {
            if (font) fonts[key] = font;
        }

        void setText(const UString& text, float fontPixelSize, Vec2f position = { 0.f, 0.f }, Vec3f color = { 1.f, 1.f, 1.f })
        {
            if (fonts.empty())
                throw std::runtime_error("No fonts attached");

            instances.clear();
            auto font = fonts.begin()->second;
            auto undefinedGlyph = font->getGlyph(0xFFFF);
            float scale = fontPixelSize / font->getCharacterPixelSize();

            Vec2f pen = position;
            float lineHeight = font->getCharacterPixelSize() * scale;
            float spaceAdvance = fontPixelSize * 0.25f;

            static Vec3f colors[9] =
            {
                {1.f, 1.f, 1.f},       // White
                {0.f, 0.f, 0.f},       // Black
                {1.0f, 0.0f, 0.0f},    // Red
                {1.0f, 0.5f, 0.0f},    // Orange
                {1.0f, 1.0f, 0.0f},    // Yellow
                {0.0f, 1.0f, 0.0f},    // Green
                {0.0f, 0.0f, 1.0f},    // Blue
                {0.29f, 0.0f, 0.51f},  // Indigo
                {0.56f, 0.0f, 1.0f}    // Violet
            };

            Vec3f penColor = color;
            bool readColor = false;
            bool escapeNext = false;

            for (auto& ch : text)
            {
                if (escapeNext)
                {
                    // просто выводим символ без интерпретации
                    escapeNext = false;
                }
                else if (readColor)
                {
                    readColor = false;
                    if (ch >= U'1' && ch <= U'9')
                    {
                        penColor = colors[ch - U'1'];
                        continue;
                    }
                    else
                    {
                        penColor = colors[0];
                    }
                }
                else if (ch == U'\\')
                {
                    escapeNext = true;
                    continue;
                }
                else if (ch == U'§')
                {
                    readColor = true;
                    continue;
                }

                if (ch == U'\n')
                {
                    pen.y += lineHeight;
                    pen.x = position.x;
                    continue;
                }
                else if (ch == U' ')
                {
                    pen.x += spaceAdvance;
                    continue;
                }

                auto glyphOpt = font->getGlyph(ch);
                if (!glyphOpt.has_value())
                    glyphOpt = undefinedGlyph;
                if (!glyphOpt.has_value())
                    continue;

                auto& g = glyphOpt.value();
                TextDetails::CharacterInstance inst{};
                inst.position.x = pen.x + g.bearing.x * scale;
                inst.position.y = pen.y - g.bearing.y * scale;
                inst.size = g.size * scale;
                inst.uvPos = g.uv.pos;
                inst.uvSize = g.uv.size;

                if (!g.colorful)
                    inst.colorMask = penColor;

                instances.push_back(inst);
                pen.x += g.advance * scale;
            }

            instanceVBO.resize(instances.size() * sizeof(TextDetails::CharacterInstance));
            instanceVBO.updateBufferData(instances);
        }


        void draw(float windowWidth, float windowHeight)
        {
            if (instances.empty()) return;

            auto font = fonts.begin()->second;


     
            shader.use();
            shader.setUniform1i("fontAtlas", 0);
            shader.setUniform2f("uScreenSize", windowWidth*2, windowHeight*2);

            font->use(0);
            TextDetails::textSampler().bind(0);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            VAO.bind();

            glDrawArraysInstanced(
                GL_TRIANGLE_STRIP,
                0,
                4,
                static_cast<GLsizei>(instances.size())
            );

            VAO.unbind();
            TextDetails::textSampler().unbind(0);
        }
    };
}
