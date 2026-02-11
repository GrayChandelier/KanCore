#pragma once
#include <glad/glad.h>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <cmath>

#include "GLTexture.hpp"

namespace KanCore::OpenGL
{
    class GLTexture2DArray
    {
    private:
        GLuint textureId = 0;
        GLsizei width = 0;
        GLsizei height = 0;
        GLsizei layers = 0;
        GLsizei levels = 0;

        static inline GLsizei mipSize(GLsizei base, GLint level) noexcept
        {
            return std::max<GLsizei>(1, base >> level);
        }

        static inline GLsizei calculateMipmapSize(GLsizei width, GLsizei height) noexcept
        {
            return 1 + static_cast<int>(std::floor(std::log2(std::max(width, height))));
        }

        void destroy() noexcept
        {
            if (textureId)
                glDeleteTextures(1, &textureId);
        }

    public:
        friend inline GLuint getGLHandle(const GLTexture2DArray& texture) noexcept { return texture.textureId; }

        GLTexture2DArray(
            GLsizei width,
            GLsizei height,
            GLsizei layers,
            TextureInternalFormat internalFormat,
            GLsizei mipLevels = 0
        ) : width(width), height(height), layers(layers)
        {
            if (width <= 0 || height <= 0 || layers <= 0)
                throw std::invalid_argument("Texture size and layers must be positive");

            if (mipLevels == 0)
                mipLevels = calculateMipmapSize(width, height);

            if (mipLevels < 1)
                throw std::invalid_argument("Texture must have at least one mip level");

            levels = mipLevels;

            glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &textureId);
            glTextureStorage3D(
                textureId,
                levels,
                static_cast<GLenum>(internalFormat),
                width,
                height,
                layers
            );

            glTextureParameteri(textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTextureParameteri(textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(textureId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(textureId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        GLsizei getLayersCount() const noexcept
        {
            return layers;
        }
        void updateLayer(
            GLint level,
            GLint layer,
            GLint x, GLint y,
            GLsizei w, GLsizei h,
            TextureDataFormat dataFormat,
            TextureDataType dataType,
            const void* data
        )
        {
            if (level < 0 || level >= levels)
                throw std::out_of_range("Mip level out of range");

            if (layer < 0 || layer >= layers)
                throw std::out_of_range("Layer index out of range");

            GLsizei mipW = mipSize(width, level);
            GLsizei mipH = mipSize(height, level);

            if (x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > mipW || y + h > mipH)
                throw std::out_of_range("Texture region out of bounds");

            GLint prevAlignment;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTextureSubImage3D(
                textureId,
                level,
                x, y, layer,
                w, h, 1,
                static_cast<GLenum>(dataFormat),
                static_cast<GLenum>(dataType),
                data
            );

            glPixelStorei(GL_UNPACK_ALIGNMENT, prevAlignment);
        }

        void setLayerData(
            GLint level,
            GLint layer,
            TextureDataFormat dataFormat,
            TextureDataType dataType,
            const void* data
        )
        {
            updateLayer(level, layer, 0, 0, mipSize(width, level), mipSize(height, level), dataFormat, dataType, data);
        }

        void generateMipmap() noexcept
        {
            if (levels > 1)
                glGenerateTextureMipmap(textureId);
        }

        void bind(uint8_t slot) const noexcept
        {
            glBindTextureUnit(slot, textureId);
        }

        GLTexture2DArray(const GLTexture2DArray&) = delete;
        GLTexture2DArray& operator=(const GLTexture2DArray&) = delete;

        GLTexture2DArray(GLTexture2DArray&& other) noexcept
            : textureId(std::exchange(other.textureId, 0)),
            width(other.width), height(other.height),
            layers(other.layers), levels(other.levels)
        {
        }

        GLTexture2DArray& operator=(GLTexture2DArray&& other) noexcept
        {
            if (this != &other)
            {
                destroy();
                textureId = std::exchange(other.textureId, 0);
                width = other.width;
                height = other.height;
                layers = other.layers;
                levels = other.levels;
            }
            return *this;
        }

        explicit operator bool() const noexcept
        {
            return static_cast<bool>(textureId);
        }

        ~GLTexture2DArray() noexcept
        {
            destroy();
        }
    };
}
