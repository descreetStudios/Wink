#include <WinkEngine/pch.hpp>
#include <WinkEngine/ECS/Components/TransformComponent.hpp>

namespace Wink::ECS
{
    void TransformComponent::set_position(const glm::vec3& p) noexcept
    {
        position = p;
        dirty = true;
    }

    void TransformComponent::set_rotation(const glm::quat& q) noexcept
    {
        rotation = glm::normalize(q);
        dirty = true;
    }

    void TransformComponent::set_euler(const glm::vec3& eulerDegrees) noexcept
    {
        rotation = glm::quat(glm::radians(eulerDegrees));
        dirty = true;
    }

    void TransformComponent::set_scale(const glm::vec3& s) noexcept
    {
        scale = s;
        dirty = true;
    }

    void TransformComponent::translate(const glm::vec3& delta) noexcept
    {
        position += delta;
        dirty = true;
    }

    void TransformComponent::rotate(const glm::quat& delta) noexcept
    {
        rotation = glm::normalize(delta * rotation);
        dirty = true;
    }

    void TransformComponent::rotate_euler(const glm::vec3& eulerDegrees) noexcept
    {
        glm::quat delta = glm::quat(glm::radians(eulerDegrees));
        rotation = glm::normalize(delta * rotation);
        dirty = true;
    }

    void TransformComponent::rotate_yaw(float degrees) noexcept
    {
        glm::quat delta = glm::angleAxis(glm::radians(degrees), glm::vec3(0.f, 1.f, 0.f));
        rotation = glm::normalize(delta * rotation);
        dirty = true;
    }

    void TransformComponent::rotate_pitch(float degrees) noexcept
    {
        glm::quat delta = glm::angleAxis(glm::radians(degrees), glm::vec3(1.f, 0.f, 0.f));
        rotation = glm::normalize(delta * rotation);
        dirty = true;
    }

    void TransformComponent::rotate_roll(float degrees) noexcept
    {
        glm::quat delta = glm::angleAxis(glm::radians(degrees), glm::vec3(0.f, 0.f, 1.f));
        rotation = glm::normalize(delta * rotation);
        dirty = true;
    }

    void TransformComponent::multiply_scale(const glm::vec3& factor) noexcept
    {
        scale *= factor;
        dirty = true;
    }

    glm::vec3 TransformComponent::get_euler() const noexcept
    {
        return glm::degrees(glm::eulerAngles(rotation));
    }

    float TransformComponent::get_yaw() const noexcept
    {
        return glm::degrees(glm::yaw(rotation));
    }

    float TransformComponent::get_pitch() const noexcept
    {
        return glm::degrees(glm::pitch(rotation));
    }

    float TransformComponent::get_roll() const noexcept
    {
        return glm::degrees(glm::roll(rotation));
    }

    glm::vec3 TransformComponent::get_forward() const noexcept
    {
        return rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    }

    glm::vec3 TransformComponent::get_right() const noexcept
    {
        return rotation * glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::vec3 TransformComponent::get_up() const noexcept
    {
        return rotation * glm::vec3(0.0f, 1.0f, 0.0f);
    }
}