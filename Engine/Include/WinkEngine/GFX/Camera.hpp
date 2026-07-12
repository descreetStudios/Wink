#pragma once

namespace Wink::GFX
{
	class Camera
	{
	public:
		glm::vec3 position = { 0.0f, 0.0f, 0.0f };
		float yaw = 0.0f;
		float pitch = 0.0f;
		float roll = 0.0f;

		float fov = 60.0f;
		float nearPlane = 0.1f;
		float farPlane = 600.0f;
		float aspectRatio = 16.f / 9.f;

	public:
		void move_axis(glm::vec3 direction,
			float dt, float speed = 5.0f);
		void look(float deltaX, float deltaY,
			float deltaRoll, float sensitivity = 0.1f);

		[[nodiscard]] glm::mat4 get_view() const;
		[[nodiscard]] glm::mat4 get_proj() const;

		[[nodiscard]] glm::vec3 get_forward() const;
		[[nodiscard]] glm::vec3 get_right() const;
		[[nodiscard]] glm::vec3 get_up() const;

	private:
		[[nodiscard]] glm::quat get_orientation() const;
	};
}