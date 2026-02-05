#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "Details.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace KanCore::Graphics
{
	class Camera;
	using Quaternion = glm::quat;
	using Matrix4x4 = glm::mat4;

	enum class CameraRotationAxis : uint8_t
	{
		AbsoluteX,
		AbsoluteY,
		AbsoluteZ,

		CameraX,
		CameraY,
		CameraZ
	};

	namespace CameraDetails
	{
		class Projection
		{
		private:
			mutable Matrix4x4 projection{ 1.0f };
			mutable bool _needsUpdate = true;

			float fov = 80.f;
			float nearestPlane = 0.2f;
			float farthestPlane = 100.f;
			float aspectRatio = 16.f/9.f;

		public:
			Projection& setClippingPlanes(float nearest, float farthest);
			Projection& setNearestPlane(float distance);
			Projection& setFarthestPlane(float distance);
			Projection& setAspectRatio(float aspect);
			Projection& setFOV(float fov);

			float getFOV() const noexcept;
			Matrix4x4 getMatrix() const noexcept;



		};
		class Transform3D
		{
		private:
			mutable bool _viewNeedsUpdate = true;
			mutable Matrix4x4 view{ 1.0f };
			Vec3f position{ 0.f, 0.f, 0.f };
			Quaternion orientation{ 1.f, 0.f, 0.f, 0.f };
		public:
			Transform3D& setPosition(Vec3f position) noexcept;
			Vec3f getPosition() const noexcept;
			Transform3D& setOrientation(const Quaternion& orientation) noexcept;
			Quaternion getOrientation() const noexcept;

			Matrix4x4 getViewMatrix() const noexcept;

			Vec3f getForwardVector() const noexcept;
			Vec3f getUpVector() const noexcept;
			Vec3f getRightVector() const noexcept;
		};

		class Controller3D
		{
		private:
			Transform3D& transform;
		public:
			Controller3D(Transform3D& transform) : transform(transform) {}
			Controller3D& move(Vec3f offset);
			Controller3D& moveAbsolute(Vec3f offset);
			Controller3D& rotate(CameraRotationAxis axis, float degrees);
			Controller3D& lookAt(Vec3f targetPosition);
		};
	}
	struct CameraUniforms
	{
		unsigned int viewUniformLocation;
		unsigned int projectionUniformLocation;
	};

	
	class Camera
	{
	public:
		CameraDetails::Transform3D transform;
		CameraDetails::Controller3D controller;
		CameraDetails::Projection projection;
	
		explicit Camera(Vec3f position, float yaw, float pitch, float roll = 0.f);
		explicit Camera(Vec3f position, Vec3f forward, Vec3f up = { 0,1,0 });
	
		void apply(const CameraUniforms& cameraUniforms);
	};
}