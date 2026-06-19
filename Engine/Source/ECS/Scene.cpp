#include <WinkEngine/pch.hpp>
#include <WinkEngine/ECS/Scene.hpp>

namespace Wink::ECS
{
	Scene::Scene(std::string name)
		: mName(std::move(name))
	{
	}

	Entity Scene::spawn()
	{
		++mEntityCount;
		return Entity(mRegistry.create(), mRegistry);
	}

	Entity Scene::wrap(EntityID id)
	{
		return Entity(id, mRegistry);
	}

	void Scene::destroy(Entity& e)
	{
		if (e) --mEntityCount;
		e.destroy();
	}

	void Scene::destroy(EntityID e)
	{
		auto entity = Entity(e, *this);
		if (entity) --mEntityCount;
		entity.destroy();
	}

	void Scene::clear_entities()
	{
		mRegistry.clear();
		mEntityCount = 0;
	}
}