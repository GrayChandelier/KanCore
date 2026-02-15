#pragma once

#include <glad/glad.h>

#include <utility>

namespace KanCore::OpenGL
{
	class GLTextureSampler
	{
	private:
		GLuint samplerId = 0;

	public:
		static void unbind(GLuint unit) noexcept
		{
			glBindSampler(unit, 0);
		}
		void bind(GLuint unit) const noexcept
		{
			glBindSampler(unit, samplerId);
		}

		void setWrap(GLint s, GLint t, GLint r)
		{
			glSamplerParameteri(samplerId, GL_TEXTURE_WRAP_S, s);
			glSamplerParameteri(samplerId, GL_TEXTURE_WRAP_T, t);
			glSamplerParameteri(samplerId, GL_TEXTURE_WRAP_R, r);
		}

		void setFilter(GLint minFilter, GLint magFilter)
		{
			glSamplerParameteri(samplerId, GL_TEXTURE_MIN_FILTER, minFilter);
			glSamplerParameteri(samplerId, GL_TEXTURE_MAG_FILTER, magFilter);
		}
		GLTextureSampler& operator=(GLTextureSampler&& other) noexcept
		{
			if (this == &other)
				return *this;

			if (samplerId)
				glDeleteSamplers(1, &samplerId);

			samplerId = std::exchange(other.samplerId, 0);

			return *this;
		}

		GLTextureSampler(GLTextureSampler&) = delete;
		GLTextureSampler(GLTextureSampler&& old) noexcept
			: samplerId(std::exchange(old.samplerId, 0)) {
		}

		GLTextureSampler()
		{
			glCreateSamplers(1, &samplerId);
			setWrap(GL_REPEAT, GL_REPEAT, GL_REPEAT);
			setFilter(GL_LINEAR, GL_LINEAR);
		}
		~GLTextureSampler() noexcept
		{
			if (samplerId)
				glDeleteSamplers(1, &samplerId);
		}
	};
}