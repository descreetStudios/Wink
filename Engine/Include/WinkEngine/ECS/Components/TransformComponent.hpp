#pragma once

#include <WinkEngine/ECS/Entity.hpp>

namespace Wink::ECS
{
	struct TransformComponent
	{
		glm::vec3 position = glm::vec3(0.0f);
		glm::quat rotation = glm::identity<glm::quat>();
		glm::vec3 scale = glm::vec3(1.0f);

		glm::mat4 worldMatrix = glm::mat4(1.0f);
		glm::mat3 normalMatrix = glm::mat3(1.0f);
		bool dirty = true;

		EntityID parent = NULL_ENTITY;
		std::vector<EntityID> children = {};

		void set_position(const glm::vec3& p) noexcept;
		void set_rotation(const glm::quat& q) noexcept;
		void set_euler(const glm::vec3& eulerDegrees) noexcept;
		void set_scale(const glm::vec3& s) noexcept;

		void translate(const glm::vec3& delta) noexcept;
		void rotate(const glm::quat& delta) noexcept;
		void rotate_euler(const glm::vec3& eulerDegrees) noexcept;
		void rotate_yaw(float degrees) noexcept;
		void rotate_pitch(float degrees) noexcept;
		void rotate_roll(float degrees) noexcept;
		void multiply_scale(const glm::vec3& factor) noexcept;

		glm::vec3 get_euler() const noexcept;
		float get_yaw() const noexcept;
		float get_pitch() const noexcept;
		float get_roll() const noexcept;

		glm::vec3 get_forward() const noexcept;
		glm::vec3 get_right() const noexcept;
		glm::vec3 get_up() const noexcept;
	};
}