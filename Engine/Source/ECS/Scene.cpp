#include <WinkEngine/pch.hpp>
#include <WinkEngine/ECS/Scene.hpp>
#include <WinkEngine/Core/Logger.hpp>

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

	namespace
	{
		std::vector<std::unique_ptr<Scene>> gScenes;
		Scene* gActiveScene = nullptr;

		Scene* find_scene(std::string_view name) noexcept
		{
			for (auto& s : gScenes)
				if (s->get_name() == name) return s.get();
			return nullptr;
		}
	} // anonymous namespace

	Scene* create_scene(std::string name)
	{
		if (find_scene(name))
		{
			Logger::Internal::error(
				"Scene already exists: '{}'", name);
			return nullptr;
		}

		gScenes.push_back(std::make_unique<Scene>(std::move(name)));

		if (gActiveScene == nullptr)
			gActiveScene = gScenes.back().get();

		return gScenes.back().get();
	}

	void destroy_scene(Scene& scene)
	{
		auto it = std::find_if(gScenes.begin(), gScenes.end(),
			[&](const auto& s) { return s.get() == &scene; });

		if (it == gScenes.end())
		{
			Logger::Internal::error("Scene not found");
			return;
		}

		if (gActiveScene == &scene)
			gActiveScene = nullptr;

		gScenes.erase(it);
	}

	void destroy_scene(std::string_view name)
	{
		auto it = std::find_if(gScenes.begin(), gScenes.end(),
			[&](const auto& s) { return s->get_name() == name; });

		if (it == gScenes.end())
		{
			Logger::Internal::error("Scene not found: '{}'", name);
			return;
		}

		destroy_scene(**it);
	}

	void clear_scenes()
	{
		gActiveScene = nullptr;
		gScenes.clear();
	}

	Scene* get_scene(std::string_view name) noexcept
	{
		return find_scene(name);
	}

	void set_active_scene(Scene* scene) noexcept
	{
		gActiveScene = scene;
	}

	Scene* set_active_scene(std::string_view name)
	{
		auto* scene = get_scene(name);
		gActiveScene = scene;
		return scene;
	}

	bool has_active_scene() noexcept
	{
		return gActiveScene != nullptr;
	}

	Scene* get_active_scene() noexcept
	{
		return gActiveScene;
	}
}