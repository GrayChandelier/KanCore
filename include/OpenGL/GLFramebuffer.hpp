#pragma once
#include <glad/glad.h>
#include <utility>
#include <stdexcept>
#include "GLTexture.hpp"

namespace KanCore::OpenGL
{
	enum class FramebufferMode : GLenum
	{
		Read = GL_READ_FRAMEBUFFER,
		Draw = GL_DRAW_FRAMEBUFFER,
		ReadDraw = GL_FRAMEBUFFER
	};
	enum class FramebufferAttachment : GLenum
	{
		Color0 = GL_COLOR_ATTACHMENT0,
		Color1 = GL_COLOR_ATTACHMENT1,
		Color2 = GL_COLOR_ATTACHMENT2,
		Color3 = GL_COLOR_ATTACHMENT3,
		Color4 = GL_COLOR_ATTACHMENT4,
		Color5 = GL_COLOR_ATTACHMENT5,
		Color6 = GL_COLOR_ATTACHMENT6,
		Color7 = GL_COLOR_ATTACHMENT7,
		Color8 = GL_COLOR_ATTACHMENT8,
		Color9 = GL_COLOR_ATTACHMENT9,
		Color10 = GL_COLOR_ATTACHMENT10,
		Color11 = GL_COLOR_ATTACHMENT11,
		Color12 = GL_COLOR_ATTACHMENT12,
		Color13 = GL_COLOR_ATTACHMENT13,
		Color14 = GL_COLOR_ATTACHMENT14,
		Color15 = GL_COLOR_ATTACHMENT15,

		// Depth / stencil
		Depth = GL_DEPTH_ATTACHMENT,
		Stencil = GL_STENCIL_ATTACHMENT,
		DepthStencil = GL_DEPTH_STENCIL_ATTACHMENT
	};


	class GLFramebuffer
	{
	private:
		GLuint framebufferId = 0;

		void destroy() noexcept
		{
			if (framebufferId)
				glDeleteFramebuffers(1, &framebufferId);
		}

		void checkComplete() const
		{
			if (!framebufferId) 
				throw std::runtime_error("Framebuffer not created");

			if (glCheckNamedFramebufferStatus(framebufferId, GL_FRAMEBUFFER)
				!= GL_FRAMEBUFFER_COMPLETE)
				throw std::runtime_error("Framebuffer incomplete");
		}
	public:
		friend inline GLuint getGLHandle(const GLFramebuffer& framebuffer) noexcept { return framebuffer.framebufferId; }

		void attach(FramebufferAttachment attachment, GLTexture2D& texture, GLuint mip = 0)
		{
			if (!framebufferId) 
				throw std::runtime_error("Framebuffer not created");

			GLuint textureId = getGLHandle(texture);
			
			if(!textureId)
				throw std::runtime_error("Texture not created");

			glNamedFramebufferTexture(framebufferId, static_cast<GLenum>(attachment), textureId, mip);

	
		}
		GLFramebuffer()
		{
			glCreateFramebuffers(1, &framebufferId);
	
		}
		GLFramebuffer(GLFramebuffer&) = delete;
		GLFramebuffer(GLFramebuffer&& old) noexcept
			: framebufferId(std::exchange(old.framebufferId, 0)) { }
		
		GLFramebuffer& operator=(GLFramebuffer&& old) noexcept
		{
			if (this != &old)
			{
				destroy();
				framebufferId = std::exchange(old.framebufferId, 0);
			}
			return *this;
		}

		void bind(FramebufferMode mode = FramebufferMode::ReadDraw) const noexcept
		{
			glBindFramebuffer(static_cast<GLenum>(mode), framebufferId);
		}

		static void bindDefault() noexcept
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		
		explicit operator bool() const noexcept
		{
			return static_cast<bool>(framebufferId);
		}

		~GLFramebuffer()
		{
			destroy();
		}
	};
}