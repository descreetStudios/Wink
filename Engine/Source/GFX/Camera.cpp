#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Camera.hpp>

namespace Wink::GFX
{
	static constexpr glm::vec3 UP = { 0.0f, 1.0f, 0.0f };

	void Camera::move_axis(
		glm::vec3 direction,
		float dt, float speed)
	{
		assert(dt >= 0.0f);

		if (glm::length(direction) > 0.0f)
			position += glm::normalize(direction) * speed * dt;
	}

	void Camera::look(float deltaX, float deltaY,
		float deltaRoll, float sensitivity)
	{
		yaw += deltaX * sensitivity;
		pitch -= deltaY * sensitivity;
		roll += deltaRoll * sensitivity;

		pitch = glm::clamp(pitch, -89.0f, 89.0f);
	}

	glm::quat Camera::get_orientation() const
	{
		glm::quat qYaw = glm::angleAxis(glm::radians(yaw), glm::vec3(0, 1, 0));
		glm::quat qPitch = glm::angleAxis(glm::radians(pitch), glm::vec3(1, 0, 0));
		glm::quat qRoll = glm::angleAxis(glm::radians(roll), glm::vec3(0, 0, 1));

		return qYaw * qPitch * qRoll;
	}

	glm::mat4 Camera::get_view() const
	{
		return glm::lookAt(position, position + get_forward(), UP);
	}

	glm::mat4 Camera::get_proj() const
	{
		assert(aspectRatio > 0.0f);
		assert(farPlane > nearPlane);

		return glm::perspective(glm::radians(fov),
			aspectRatio, nearPlane, farPlane);
	}

	glm::vec3 Camera::get_forward() const
	{
		return glm::normalize(get_orientation() * glm::vec3(0, 0, -1));
	}

	glm::vec3 Camera::get_right() const
	{
		return glm::normalize(get_orientation() * glm::vec3(1, 0, 0));
	}

	glm::vec3 Camera::get_up() const
	{
		return glm::normalize(get_orientation() * glm::vec3(0, 1, 0));
	}
}