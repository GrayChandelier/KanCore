#pragma once
#include <glm/vec3.hpp>

namespace KanCore
{
    using Vec3f = glm::vec3;
    using Vec3i = glm::ivec3;

    class Vec2i
    {
    public:
        int x = 0;
        int y = 0;

        Vec2i() = default;
        Vec2i(int x, int y) : x(x), y(y) {}
        Vec2i(const Vec2i& other) = default;

        Vec2i& operator=(const Vec2i& other) = default;

        bool operator==(const Vec2i& other) const
        {
            return x == other.x && y == other.y;
        }
        bool operator!=(const Vec2i& other) const 
        { 
            return !(*this == other); 
        }

        Vec2i operator+(const Vec2i& other) const
        {
            return Vec2i(x + other.x, y + other.y);
        }

        Vec2i& operator+=(const Vec2i& other)
        {
            x += other.x;
            y += other.y;
            return *this;
        }

        Vec2i operator-(const Vec2i& other) const
        {
            return Vec2i(x - other.x, y - other.y);
        }

        Vec2i& operator-=(const Vec2i& other)
        {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        int operator*(const Vec2i& other) const 
        {
            return x * other.x + y * other.y;
        }
        Vec2i operator*(int scalar) const { return Vec2i(x * scalar, y * scalar); }
        Vec2i& operator*=(int scalar) { x *= scalar; y *= scalar; return *this; }
    };
    class Vec2f
    {
    public:
        float x = 0.0f;
        float y = 0.0f;

        Vec2f() = default;
        Vec2f(float x, float y) : x(x), y(y) {}
        Vec2f(const Vec2f& other) = default;

        Vec2f& operator=(const Vec2f& other) = default;

        bool operator==(const Vec2f& other) const
        {
            return x == other.x && y == other.y;
        }
        bool operator!=(const Vec2f& other) const 
        {
            return !(*this == other); 
        }
        Vec2f operator+(const Vec2f& other) const
        {
            return Vec2f(x + other.x, y + other.y);
        }

        Vec2f& operator+=(const Vec2f& other)
        {
            x += other.x;
            y += other.y;
            return *this;
        }

        Vec2f operator-(const Vec2f& other) const
        {
            return Vec2f(x - other.x, y - other.y);
        }

        Vec2f& operator-=(const Vec2f& other)
        {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        float operator*(const Vec2f& other) const
        {
            return x * other.x + y * other.y;
        }
        Vec2f operator*(float scalar) const { return Vec2f(x * scalar, y * scalar); }
        Vec2f& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
    };

    struct Size2D
    {
        float width = 0.0f;
        float height = 0.0f;

        Size2D() = default;
        Size2D(float w, float h) : width(w), height(h) {}
    };
}
