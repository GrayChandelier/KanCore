#pragma once
#include <glad/glad.h>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <cmath>
namespace KanCore::OpenGL
{
    enum class TextureInternalFormat : GLenum
    {
        R8 = GL_R8,
        RG8 = GL_RG8,
        RGB8 = GL_RGB8,
        RGBA8 = GL_RGBA8,

        SRGB8 = GL_SRGB8,
        SRGB8_ALPHA8 = GL_SRGB8_ALPHA8,

        R16F = GL_R16F,
        RG16F = GL_RG16F,
        RGB16F = GL_RGB16F,
        RGBA16F = GL_RGBA16F,

        R32F = GL_R32F,
        RG32F = GL_RG32F,
        RGB32F = GL_RGB32F,
        RGBA32F = GL_RGBA32F,

        DEPTH24 = GL_DEPTH_COMPONENT24,
        DEPTH32F = GL_DEPTH_COMPONENT32F,
        DEPTH24_STENCIL8 = GL_DEPTH24_STENCIL8
    };

    enum class TextureDataFormat : GLenum
    {
        R = GL_RED,
        RG = GL_RG,
        RGB = GL_RGB,
        BGR = GL_BGR,
        RGBA = GL_RGBA,
        BGRA = GL_BGRA,

        DEPTH_COMPONENT = GL_DEPTH_COMPONENT,
        DEPTH_STENCIL = GL_DEPTH_STENCIL
    };

    enum class TextureDataType : GLenum
    {
        UBYTE = GL_UNSIGNED_BYTE,
        BYTE = GL_BYTE,
        USHORT = GL_UNSIGNED_SHORT,
        SHORT = GL_SHORT,
        UINT = GL_UNSIGNED_INT,
        INT = GL_INT,
        FLOAT = GL_FLOAT,
        HALF_FLOAT = GL_HALF_FLOAT,
        DEPTH24_STENCIL8 = GL_UNSIGNED_INT_24_8
    };

    inline OpenGL::TextureInternalFormat getInternalFormat(uint8_t channels)
    {
        switch (channels)
        {
        case 1: return OpenGL::TextureInternalFormat::R8;
        case 2: return OpenGL::TextureInternalFormat::RG8;
        case 3: return OpenGL::TextureInternalFormat::RGB8;
        case 4: return OpenGL::TextureInternalFormat::RGBA8;
        default: throw std::runtime_error("Unsupported channel count");
        }
    }

    inline OpenGL::TextureDataFormat getDataFormat(uint8_t channels)
    {
        switch (channels)
        {
        case 1: return OpenGL::TextureDataFormat::R;
        case 2: return OpenGL::TextureDataFormat::RG;
        case 3: return OpenGL::TextureDataFormat::RGB;
        case 4: return OpenGL::TextureDataFormat::RGBA;
        default: throw std::runtime_error("Unsupported channel count");
        }
    }


    class GLTexture2D
    {
    private:
        GLuint  textureId = 0;
        GLsizei width = 0;
        GLsizei height = 0;
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
        friend inline GLuint getGLHandle(const GLTexture2D& texture) noexcept { return texture.textureId; }

        GLTexture2D(
            GLsizei width,
            GLsizei height,
            TextureInternalFormat internalFormat,
            GLsizei mipLevels = 0
        )
            : width(width), height(height)
        {
            if (width <= 0 || height <= 0)
                throw std::invalid_argument("Texture size must be positive");

            if (mipLevels == 0)
                mipLevels = calculateMipmapSize(width, height);
            

            if (mipLevels < 1)
                throw std::invalid_argument("Texture must have at least one mip level");

            levels = mipLevels;

            glCreateTextures(GL_TEXTURE_2D, 1, &textureId);
            glTextureStorage2D(
                textureId,
                levels,
                static_cast<GLenum>(internalFormat),
                width,
                height
            );
        }

        void updateRegion(
            GLint level,
            GLint x, GLint y,
            GLsizei width, GLsizei height,
            TextureDataFormat dataFormat,
            TextureDataType dataType,
            const void* data
        )
        {
            if (level < 0 || level >= levels)
                throw std::out_of_range("Mip level out of range");

            GLsizei mipW = mipSize(this->width, level);
            GLsizei mipH = mipSize(this->height, level);

            if (x < 0 || y < 0 || width <= 0 || height <= 0 ||
                x + width > mipW || y + height > mipH)
                throw std::out_of_range("Texture region out of bounds");

            GLint prevAlignment;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTextureSubImage2D(
                textureId,
                level,
                x, y,
                width, height,
                static_cast<GLenum>(dataFormat),
                static_cast<GLenum>(dataType),
                data
            );

            glPixelStorei(GL_UNPACK_ALIGNMENT, prevAlignment);
        }

        //Convenience wrapper
        void setData(
            GLint level,
            TextureDataFormat dataFormat,
            TextureDataType dataType,
            const void* data
        )
        {
            updateRegion(
                level,
                0, 0,
                mipSize(width, level),
                mipSize(height, level),
                dataFormat,
                dataType,
                data
            );
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

        GLTexture2D(const GLTexture2D&) = delete;
        GLTexture2D& operator=(const GLTexture2D&) = delete;

        GLTexture2D(GLTexture2D&& other) noexcept
            : textureId(std::exchange(other.textureId, 0)),
            width(other.width),
            height(other.height),
            levels(other.levels)
        {
        }

        GLTexture2D& operator=(GLTexture2D&& other) noexcept
        {
            if (this != &other)
            {
                destroy();

                textureId = std::exchange(other.textureId, 0);
                width = other.width;
                height = other.height;
                levels = other.levels;
            }
            return *this;
        }

        explicit operator bool() const noexcept
        {
            return static_cast<bool>(textureId);
        }
        ~GLTexture2D() noexcept
        {
            destroy();
        }
    };
}
