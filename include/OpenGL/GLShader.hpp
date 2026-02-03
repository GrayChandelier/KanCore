#pragma once
#include <glad/glad.h>
#include <utility>
#include <string>
#include <stdexcept>
#include <initializer_list>

namespace KanCore::OpenGL
{
	enum class GLShaderType : GLenum
	{
		FRAGMENT = GL_FRAGMENT_SHADER,
		VERTEX = GL_VERTEX_SHADER,
		COMPUTE = GL_COMPUTE_SHADER,
		GEOMETRY = GL_GEOMETRY_SHADER,
		TESSELATION_CONTROL = GL_TESS_CONTROL_SHADER,
		TESSELATION_EVALUATION = GL_TESS_EVALUATION_SHADER
	};

	class GLShaderException : public std::runtime_error
	{
	public:
		GLShaderException(const std::string& message) : runtime_error(message)
		{ }
	};

	class GLShader
	{
	private:
		GLuint shaderId = 0;
		GLShaderType shaderType;
		inline void destroy() noexcept
		{
			if (shaderId)
				glDeleteShader(shaderId);
		}

		inline void checkShader()
		{
			GLint success = 0;
			glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				char buffer[2048];
				glGetShaderInfoLog(shaderId, sizeof(buffer), nullptr, buffer);
				std::string info(buffer);
				throw GLShaderException("Shader compile error: " + info);
			}
		}


	public:
		GLuint getShaderId() const noexcept
		{
			return shaderId;
		}
		GLShaderType getShaderType() const noexcept
		{
			return shaderType;
		}

		GLShader(const std::string& source, GLShaderType type)
			: shaderType(type)
		{
			shaderId = glCreateShader(static_cast<GLenum>(type));

			const char* sourcePtr = source.c_str();
			glShaderSource(shaderId, 1, &sourcePtr, nullptr);
			glCompileShader(shaderId);

			checkShader();
		}

		GLShader(GLShader&) = delete;
		GLShader(GLShader&& other) noexcept
			: shaderId(std::exchange(other.shaderId, 0)), shaderType(other.shaderType) { }
		GLShader& operator=(GLShader&& other) noexcept
		{
			if (this != &other)
			{
				destroy();
				shaderId = std::exchange(other.shaderId, 0);
			}

			return *this;
		}
		~GLShader() noexcept
		{
			destroy();
		}
	};


	using ShaderList = std::initializer_list<std::reference_wrapper<GLShader>>;
	class GLShaderProgram
	{
		friend void linkShaderProgram(GLShaderProgram& program, ShaderList shaders);
	private:
		GLuint programId = 0;
		inline void destroy() noexcept
		{
			if (programId)
				glDeleteProgram(programId);
		}

		void attachShader(GLShader& shader)
		{
			glAttachShader(programId, shader.getShaderId());
		}
		void link()
		{
			glLinkProgram(programId);

			GLint link_ok = 0;
			glGetProgramiv(programId, GL_LINK_STATUS, &link_ok);
			if (!link_ok)
			{
				char buffer[2048];
				glGetProgramInfoLog(programId, sizeof(buffer), nullptr, buffer);
				std::string info(buffer);
				throw GLShaderException("Shader compile error: " + info);
			}
		}


	public:
		GLShaderProgram() 
			: programId(0)
		{
			
		}
		void use() const noexcept
		{
			glUseProgram(programId);
		}
		static void unbindShaders() noexcept
		{
			glUseProgram(0);
		}
		GLShaderProgram(GLShaderProgram&) = delete;
		GLShaderProgram(GLShaderProgram&& other) noexcept
			: programId(std::exchange(other.programId, 0)) {
		}
		GLShaderProgram& operator=(GLShaderProgram&& other) noexcept
		{
			if (this != &other)
			{
				destroy();
				programId = std::exchange(other.programId, 0);
			}

			return *this;
		}
		~GLShaderProgram() noexcept
		{
			destroy();
		}

	};


	void linkShaderProgram(GLShaderProgram& program, ShaderList shaders)
	{
		program.programId = glCreateProgram();
		for (auto& shader : shaders)
		{
			program.attachShader(shader);
		}
		program.link();
	}
}