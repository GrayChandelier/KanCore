#pragma once

#include <glad/glad.h>

#include <utility>

namespace KanCore::OpenGL
{


	class GLRenderbuffer;
	class GLFramebuffer;
	class GLPixelbuffer;

	class GLTexture2D
	{
	private:
		GLuint textureId = 0;
	public:
		void setData(GLenum format, GLsizei width, GLsizei height, const void* data) noexcept
		{
			glTextureStorage2D(textureId, 1, format, width, height);
			glTextureSubImage2D(textureId, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
		}
		void updateRegion(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, const void* data) noexcept
		{
			glTextureSubImage2D(textureId, 0, x, y, width, height, format, GL_UNSIGNED_BYTE, data);
		}
		void generateMipmap() noexcept
		{
			glGenerateTextureMipmap(textureId);

		}
		void bind(uint8_t textureSlot) const noexcept
		{
			glBindTextureUnit(textureSlot, textureId);
		}
		GLTexture2D(GLTexture2D&) = delete;
		GLTexture2D(GLTexture2D&& old) noexcept
		{
			textureId = std::exchange(old.textureId, 0);
		}
		GLTexture2D& operator=(GLTexture2D&& other) noexcept
		{
			if (this == &other)
				return *this;

			if (textureId)
				glDeleteTextures(1, &textureId);

			textureId = std::exchange(other.textureId, 0);

			return *this;
		}
		GLTexture2D()
		{
			glCreateTextures(GL_TEXTURE_2D, 1, &textureId);
		}
		~GLTexture2D() noexcept
		{
			if (textureId)
				glDeleteTextures(1, &textureId);
		}
	};
}