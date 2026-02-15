#pragma once
#include <GLAD/glad.h>

#include <utility>
#include <span>
#include <stdexcept>
#include <concepts>

namespace KanCore::OpenGL::Details
{
	class GL_VBO_RAII
	{
	private:
		GLuint vertexBufferId = 0;

		void destory() noexcept
		{
			if (vertexBufferId)
				glDeleteBuffers(1, &vertexBufferId);
		}
	public:
		GLuint getId() const
		{
			return vertexBufferId;
		}
		GL_VBO_RAII(GL_VBO_RAII&) = delete;
		GL_VBO_RAII(GL_VBO_RAII&& old) noexcept
			: vertexBufferId(std::exchange(old.vertexBufferId, 0))
		{
		}
		GL_VBO_RAII& operator=(GL_VBO_RAII&& other) noexcept
		{
			if (this != &other)
			{
				destory();

				vertexBufferId = std::exchange(other.vertexBufferId, 0);
			}

			return *this;
		}

		GL_VBO_RAII()
		{
			glCreateBuffers(1, &vertexBufferId);
		}
		~GL_VBO_RAII() noexcept
		{
			destory();
		}

	};

	


}
namespace KanCore::OpenGL
{
	enum class GLMapAccess : GLenum
	{
		READ = GL_READ_ONLY,
		WRITE = GL_WRITE_ONLY,
		READ_AND_WRITE = GL_READ_WRITE
	};

	enum class GLUsage : GLenum
	{
		STATIC_DRAW = GL_STATIC_DRAW,
		DYNAMIC_DRAW = GL_DYNAMIC_DRAW,
		STREAM_DRAW = GL_STREAM_DRAW,
		STATIC_READ = GL_STATIC_READ,
		DYNAMIC_READ = GL_DYNAMIC_READ,
		STREAM_READ = GL_STREAM_READ,
		STATIC_COPY = GL_STATIC_COPY,
		DYNAMIC_COPY = GL_DYNAMIC_COPY,
		STREAM_COPY = GL_STREAM_COPY
	};

	enum class GLStorageFlags : GLbitfield
	{
		NONE = 0,
		DYNAMIC_STORAGE = GL_DYNAMIC_STORAGE_BIT,
		MAP_READ = GL_MAP_READ_BIT,
		MAP_WRITE = GL_MAP_WRITE_BIT,
		MAP_PERSISTENT = GL_MAP_PERSISTENT_BIT,
		MAP_COHERENT = GL_MAP_COHERENT_BIT,
		CLIENT_STORAGE = GL_CLIENT_STORAGE_BIT
	};

	template<typename T>
	concept TriviallyCopyable = std::is_trivially_copyable_v<T>;


	namespace Details
	{
		inline bool hasFlag(GLStorageFlags flags, GLStorageFlags flag) noexcept
		{
			return (static_cast<GLbitfield>(flags) & static_cast<GLbitfield>(flag)) != 0;
		}


			class GLVertexBuffer
			{
			public:
				virtual GLuint getBufferId() const noexcept = 0;
				virtual ~GLVertexBuffer() = default;
		};
		
	}

	inline GLStorageFlags operator|(GLStorageFlags a, GLStorageFlags b) noexcept
	{
		return static_cast<GLStorageFlags>(
			static_cast<GLbitfield>(a) | static_cast<GLbitfield>(b));
	}

	inline GLStorageFlags operator&(GLStorageFlags a, GLStorageFlags b) noexcept
	{
		return static_cast<GLStorageFlags>(
			static_cast<GLbitfield>(a) & static_cast<GLbitfield>(b));
	}





	template<TriviallyCopyable T>
	class GLStaticVBO : public Details::GLVertexBuffer
	{
	private:
		Details::GL_VBO_RAII vbo;
		const size_t sizeInBytes = 0;
		const GLStorageFlags flags;

		GLuint getBufferId() const noexcept override
		{
			return vbo.getId();
		}
	public:
		GLStaticVBO(GLStaticVBO&) = delete;
		GLStaticVBO(GLStaticVBO&& other) noexcept
			: vbo(std::move(other.vbo)),
			sizeInBytes(other.sizeInBytes),
			flags(other.flags)
		{
		}

		GLStaticVBO(std::span<T> data, GLStorageFlags flags = GLStorageFlags::NONE) 
			: vbo(),
			sizeInBytes(data.size_bytes()),
			flags(flags)
		{
			glNamedBufferStorage(vbo.getId(), sizeInBytes, data.data(), static_cast<GLbitfield>(flags));
		}

		GLStaticVBO(size_t sizeInBytes, GLStorageFlags flags = GLStorageFlags::NONE) 
			: vbo(), 
			sizeInBytes(sizeInBytes),
			flags(flags)
		{
			glNamedBufferStorage(vbo.getId(), sizeInBytes, nullptr, static_cast<GLbitfield>(flags));
		}

		GLStaticVBO(const void* data, size_t sizeInBytes, GLStorageFlags flags = GLStorageFlags::NONE)
			: vbo(),
			sizeInBytes(sizeInBytes),
			flags(flags)
		{
			glNamedBufferStorage(vbo.getId(), sizeInBytes, data, static_cast<GLbitfield>(flags));
		}

		size_t getSizeInBytes() const
		{
			return sizeInBytes;
		}
		void updateBufferData(std::span<T> data, size_t offsetPerBytes = 0)
		{
			if (!Details::hasFlag(flags, GLStorageFlags::DYNAMIC_STORAGE))
				throw std::runtime_error("GL buffer storgae created without DYNAMIC_STORAGE flag");
			if (data.size_bytes() + offsetPerBytes > sizeInBytes)
				throw std::out_of_range("GL buffer storage data out of range!");

			glNamedBufferSubData(vbo.getId(), offsetPerBytes, data.size_bytes(), data.data());
		}
		void updateBufferData(const void* data, size_t dataSize, size_t offsetPerBytes = 0)
		{
			if (!Details::hasFlag(flags, GLStorageFlags::DYNAMIC_STORAGE))
				throw std::runtime_error("GL buffer storgae created without DYNAMIC_STORAGE flag");
			if (dataSize + offsetPerBytes > sizeInBytes)
				throw std::out_of_range("GL buffer storage data out of range!");

			glNamedBufferSubData(vbo.getId(), offsetPerBytes, dataSize, data);
		}
		void bind() const noexcept
		{
			glBindBuffer(GL_ARRAY_BUFFER, vbo.getId());
		}
		static void unbind() noexcept
		{
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	};



	template<TriviallyCopyable T>
	class GLDynamicVBO : public Details::GLVertexBuffer
	{
	private:
		Details::GL_VBO_RAII vbo;
		size_t sizeInBytes = 0;
		GLUsage usage;

		GLuint getBufferId() const noexcept override
		{
			return vbo.getId();
		}
	public:
		GLDynamicVBO<T>(GLDynamicVBO&) = delete;
		GLDynamicVBO<T>(GLDynamicVBO&& other) noexcept
			: vbo(std::move(other.vbo)),
			sizeInBytes(other.sizeInBytes),
			usage(other.usage)
		{
		}

		GLDynamicVBO<T>(std::span<T> data, GLUsage usage) 
			: vbo(), usage(usage), sizeInBytes(data.size_bytes())
		{
			resize(sizeInBytes);
		}

		GLDynamicVBO<T>(GLUsage usage) : vbo(), usage(usage)
		{ }

		GLDynamicVBO<T>(size_t sizeInBytes, GLUsage usage) 
			: vbo(), usage(usage), sizeInBytes(sizeInBytes)
		{
			resize(sizeInBytes);
		}

		T* getMappingPointer(GLMapAccess access = GLMapAccess::READ_AND_WRITE)
		{
			return static_cast<T*>(glMapNamedBuffer(vbo.getId(), static_cast<GLenum>(access)));
		}
		void resize(size_t sizeInBytes)
		{
			this->sizeInBytes = sizeInBytes;
			glNamedBufferData(vbo.getId(), sizeInBytes, nullptr, static_cast<GLenum>(usage));
		}
		void unmap()
		{
			glUnmapNamedBuffer(vbo.getId());
		}
		void updateBufferData(std::span<T> data, size_t offsetPerBytes = 0)
		{
			glNamedBufferSubData(vbo.getId(), offsetPerBytes, data.size_bytes(), data.data());
		}

		size_t getSizeInBytes() const
		{
			return sizeInBytes;
		}

		void bind() const noexcept
		{
			glBindBuffer(GL_ARRAY_BUFFER, vbo.getId());
		}
		static void unbind() noexcept
		{
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		explicit operator bool() const noexcept
		{
			return static_cast<bool>(vbo.getId());
		}

	};
}