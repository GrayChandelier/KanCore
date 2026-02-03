#pragma once
#include "GLVertexBuffer.hpp"

#include <utility>
#include <span>
#include <stdexcept>
#include <concepts>
#include <cstddef>

namespace KanCore::OpenGL::Details
{
    class GL_VAO_RAII
    {
    private:
        GLuint vertexArrayId = 0;

    public:
        GLuint getId() const noexcept
        {
            return vertexArrayId;
        }

        GL_VAO_RAII(const GL_VAO_RAII&) = delete;

        GL_VAO_RAII(GL_VAO_RAII&& old) noexcept
            : vertexArrayId(std::exchange(old.vertexArrayId, 0))
        {
        }

        GL_VAO_RAII& operator=(GL_VAO_RAII&& other) noexcept
        {
            if (this != &other)
            {
                destroy();
                vertexArrayId = std::exchange(other.vertexArrayId, 0);
            }
            return *this;
        }

        GL_VAO_RAII() noexcept
        {
            glCreateVertexArrays(1, &vertexArrayId);
        }

        ~GL_VAO_RAII() noexcept
        {
            destroy();
        }

        explicit operator bool() const noexcept
        {
            return vertexArrayId != 0;
        }

    private:
        void destroy() noexcept
        {
            if (vertexArrayId)
                glDeleteVertexArrays(1, &vertexArrayId);
        }
    };
}

namespace KanCore::OpenGL
{
    enum class GLAttributeType : GLenum
    {
        BYTE = GL_BYTE,
        UNSIGNED_BYTE = GL_UNSIGNED_BYTE,
        SHORT = GL_SHORT,
        UNSIGNED_SHORT = GL_UNSIGNED_SHORT,
        INT = GL_INT,
        UNSIGNED_INT = GL_UNSIGNED_INT,
        HALF_FLOAT = GL_HALF_FLOAT,
        FLOAT = GL_FLOAT,
        DOUBLE = GL_DOUBLE,
        FIXED = GL_FIXED,
        INT_2_10_10_10_REV = GL_INT_2_10_10_10_REV,
        UNSIGNED_INT_2_10_10_10_REV = GL_UNSIGNED_INT_2_10_10_10_REV
    };

    class GLVertexArray
    {
    private:
        Details::GL_VAO_RAII vao;

        inline void _attachVertexBuffer(GLuint bindingIndex, GLuint bufferId, GLintptr offset = 0, GLsizei stride = 0) 
        { 
            glVertexArrayVertexBuffer(vao.getId(), bindingIndex, bufferId, offset, stride); 
        }

    public:
        GLVertexArray() noexcept = default;

        GLVertexArray(const GLVertexArray&) = delete;
        GLVertexArray& operator=(const GLVertexArray&) = delete;

        GLVertexArray(GLVertexArray&&) noexcept = default;
        GLVertexArray& operator=(GLVertexArray&&) noexcept = default;

        template<TriviallyCopyable T>
        void attachVertexBuffer(GLuint bindingIndex,
            GLDynamicVBO<T>& vbo,
            GLintptr offset = 0,
            GLsizei stride = 0)
        {
            _attachVertexBuffer(bindingIndex, static_cast<Details::GLVertexBuffer&>(vbo).getBufferId(), offset,
                stride > 0 ? stride : sizeof(T));
        }

        template<TriviallyCopyable T>
        void attachVertexBuffer(GLuint bindingIndex,
            GLStaticVBO<T>& vbo,
            GLintptr offset = 0,
            GLsizei stride = 0)
        {
            _attachVertexBuffer(bindingIndex, static_cast<Details::GLVertexBuffer&>(vbo).getBufferId(), offset,
                stride > 0 ? stride : sizeof(T));
        }



        void setAttributeFormat(GLuint attributeIndex,
            GLint componentsCount,
            GLAttributeType type,
            GLboolean normalized,
            GLuint relativeOffset)
        {
            glVertexArrayAttribFormat(vao.getId(), attributeIndex, componentsCount,
                static_cast<GLenum>(type), normalized,
                relativeOffset);
        }

        void setAttributeBinding(GLuint attributeIndex, GLuint bindingIndex)
        {
            glVertexArrayAttribBinding(vao.getId(), attributeIndex, bindingIndex);
        }

        void enableAttribute(GLuint attributeIndex)
        {
            glEnableVertexArrayAttrib(vao.getId(), attributeIndex);
        }

        void disableAttribute(GLuint attributeIndex)
        {
            glDisableVertexArrayAttrib(vao.getId(), attributeIndex);
        }

        void setBindingDivisor(GLuint bindingIndex, GLuint divisor)
        {
            glVertexArrayBindingDivisor(vao.getId(), bindingIndex, divisor);
        }

        template<typename T>
        void attachElementBuffer(GLStaticVBO<T>& ebo)
        {
            glVertexArrayElementBuffer(vao.getId(), static_cast<Details::GLVertexBuffer&>(ebo).getBufferId());
        }

        template<typename T>
        void attachElementBuffer(GLDynamicVBO<T>& ebo)
        {
            glVertexArrayElementBuffer(vao.getId(), static_cast<Details::GLVertexBuffer&>(ebo).getBufferId());
        }

        void bind() const noexcept
        {
            glBindVertexArray(vao.getId());
        }

        static void unbind() noexcept
        {
            glBindVertexArray(0);
        }

        explicit operator bool() const noexcept
        {
            return static_cast<bool>(vao);
        }
    };  
}