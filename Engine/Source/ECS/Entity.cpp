#include <WinkEngine/pch.hpp>
#include <WinkEngine/ECS/Entity.hpp>
#include <WinkEngine/ECS/Scene.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::ECS
{
	Entity::Entity(EntityID id, Registry& reg) noexcept
		: mID(id), mReg(&reg)
	{
	}

	Entity::Entity(IDType id, Registry& reg) noexcept
		: mID(static_cast<EntityID>(id))
	{
	}

	Entity::Entity(EntityID id, Scene& scene) noexcept
		: mID(id), mReg(&scene.get_registry())
	{
	}

	Entity::Entity(IDType id, Scene& scene) noexcept
		: mID(static_cast<EntityID>(id)), mReg(&scene.get_registry())
	{
	}

	void Entity::assert_valid() const
	{
		if (!is_valid())
			Logger::critical("Operating on invalid entity");
	}

	void Entity::destroy()
	{
		assert_valid();
		mReg->destroy(mID);
		mID = NULL_ENTITY;
	}
}