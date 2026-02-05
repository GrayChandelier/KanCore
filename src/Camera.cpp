#include "../include/Camera.hpp"
#include <glad/glad.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp> 



namespace KanCore::Graphics::CameraDetails
{
    Projection& Projection::setClippingPlanes(float nearest, float farthest)
    {
        nearestPlane = nearest;
        farthestPlane = farthest;
        _needsUpdate = true;
        return *this;
    }

    Projection& Projection::setNearestPlane(float distance)
    {
        nearestPlane = distance;
        _needsUpdate = true;
        return *this;
    }

    Projection& Projection::setFarthestPlane(float distance)
    {
        farthestPlane = distance;
        _needsUpdate = true;
        return *this;
    }

    Projection& Projection::setAspectRatio(float aspect)
    {
        aspectRatio = aspect;
        _needsUpdate = true;
        return *this;
    }

    Projection& Projection::setFOV(float degrees)
    {
        fov = degrees;
        _needsUpdate = true;
        return *this;
    }

    float Projection::getFOV() const noexcept
    {
        return fov;
    }

    Matrix4x4 Projection::getMatrix() const noexcept
    {
        if (_needsUpdate)
        {
            projection = glm::perspective(
                glm::radians(fov),
                aspectRatio,
                nearestPlane,
                farthestPlane
            );

            _needsUpdate = false;
        }
        return projection;
    }

    Transform3D& Transform3D::setPosition(Vec3f pos) noexcept
    {
        position = pos;
        _viewNeedsUpdate = true;
        return *this;
    }

    Vec3f Transform3D::getPosition() const noexcept
    {
        return position;
    }

    Transform3D& Transform3D::setOrientation(const Quaternion& orientation) noexcept
    {
        this->orientation = glm::normalize(orientation);
        _viewNeedsUpdate = true;
        return *this;
    }

    Quaternion Transform3D::getOrientation() const noexcept
    {
        return orientation;
    }


    Matrix4x4 Transform3D::getViewMatrix() const noexcept
    {
        if (_viewNeedsUpdate)
        {
            view = glm::mat4_cast(orientation) * glm::translate(Matrix4x4(1.0f), -position);
            _viewNeedsUpdate = false;
        }
        return view;
    }

    Vec3f Transform3D::getForwardVector() const noexcept
    {
        return glm::normalize(orientation * Vec3f(0, 0, -1));
    }

    Vec3f Transform3D::getUpVector() const noexcept
    {
        return glm::normalize((orientation * Vec3f(0, 1, 0)));
    }

    Vec3f Transform3D::getRightVector() const noexcept
    {
        return glm::normalize((orientation * Vec3f(1, 0, 0)));
    }

    Controller3D& Controller3D::move(Vec3f offset)
    {
        transform.setPosition(
            transform.getPosition() +
            transform.getRightVector() * offset.x +
            transform.getUpVector() * offset.y +
            transform.getForwardVector() * offset.z);
        return *this;
    }

    Controller3D& Controller3D::moveAbsolute(Vec3f offset)
    {
        transform.setPosition(transform.getPosition() + offset);
        return *this;
    }

    Controller3D& Controller3D::rotate(CameraRotationAxis axis, float degrees)
    {
        float rad = glm::radians(degrees);
        glm::vec3 axisVec;

        switch (axis)
        {
            case CameraRotationAxis::AbsoluteX: axisVec = glm::vec3(1, 0, 0); break;
            case CameraRotationAxis::AbsoluteY: axisVec = glm::vec3(0, 1, 0); break;
            case CameraRotationAxis::AbsoluteZ: axisVec = glm::vec3(0, 0, 1); break;
            case CameraRotationAxis::CameraX: axisVec = transform.getRightVector(); break;
            case CameraRotationAxis::CameraY: axisVec = transform.getUpVector(); break;
            case CameraRotationAxis::CameraZ: axisVec = transform.getForwardVector(); break;
        }

        Quaternion current = transform.getOrientation();
        Quaternion delta = glm::angleAxis(rad, axisVec);
        Quaternion newOrientation = glm::normalize(delta * current);

        transform.setOrientation(newOrientation);
        return *this;
    }

    Controller3D& Controller3D::lookAt(Vec3f targetPosition)
    {
        glm::vec3 direction;

        if (targetPosition == transform.getPosition())
            direction = Vec3f{ 0,-1.f,0 };
        else
            direction = glm::normalize(targetPosition - transform.getPosition());

        Quaternion targetOri = glm::quatLookAt(direction, glm::vec3(0, 1, 0));

        transform.setOrientation(targetOri);
        return *this;
    }

    

}

namespace KanCore::Graphics
{
    Camera::Camera(Vec3f position, float yaw, float pitch, float roll)
        : transform()
        , controller(transform)
        , projection()
    {
        transform.setPosition(position);

        Quaternion qYaw = glm::angleAxis(glm::radians(yaw), glm::vec3(0, 1, 0));
        Quaternion qPitch = glm::angleAxis(glm::radians(pitch), glm::vec3(1, 0, 0));
        Quaternion qRoll = glm::angleAxis(glm::radians(roll), glm::vec3(0, 0, 1));

        transform.setOrientation(qYaw * qPitch * qRoll);
    }

    Camera::Camera(Vec3f position, Vec3f forward, Vec3f up)
        : transform()
        , controller(transform)
        , projection()
    {
        transform.setPosition(position);

        glm::vec3 dir = glm::normalize(forward);
        transform.setOrientation(glm::quatLookAt(dir, up));
    }

    void Camera::apply(const CameraUniforms& uniforms)
    {
        Matrix4x4 p = projection.getMatrix();
        Matrix4x4 v = transform.getViewMatrix();

        glUniformMatrix4fv(uniforms.projectionUniformLocation, 1, GL_FALSE, glm::value_ptr(p));
        glUniformMatrix4fv(uniforms.viewUniformLocation, 1, GL_FALSE, glm::value_ptr(v));
    }
}