#pragma once

#include <WinkEngine/ECS/Entity.hpp>

namespace Wink::ECS
{
	class Scene
	{
	public:
		explicit Scene(std::string name);
		~Scene() = default;

		// Non-copyable, movable
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;
		Scene(Scene&&) = default;
		Scene& operator=(Scene&&) = default;

		[[nodiscard]] Entity spawn();
		[[nodiscard]] Entity wrap(EntityID id);

		/* Views */
		template <Component... Cs>
		[[nodiscard]] auto view() { return mRegistry.view<Cs...>().each(); }

		template <Component... Cs>
		[[nodiscard]] auto view() const { return mRegistry.view<const Cs...>().each(); }

		template <Component... Cs, Component... Ex>
		[[nodiscard]] auto view_ex(entt::exclude_t<Ex...> excl = {})
		{
			return mRegistry.view<Cs...>(excl).each();
		}

		template <Component... Cs>
		[[nodiscard]] std::optional<Entity> find_first()
		{
			for (auto&& tuple : mRegistry.view<Cs...>().each())
				return Entity(std::get<0>(tuple), mRegistry);
			return std::nullopt;
		}

		template <std::invocable<Entity> Fn>
		void each(Fn&& fn)
		{
			for (auto id : mRegistry.storage<EntityID>())
				fn(Entity(id, mRegistry));
		}

		void destroy(Entity& e);
		void destroy(EntityID e);
		void clear_entities();

		[[nodiscard]] const std::string& get_name() const noexcept { return mName; }
		[[nodiscard]] Registry& get_registry() noexcept { return mRegistry; }
		[[nodiscard]] const Registry& get_registry() const noexcept { return mRegistry; }
		[[nodiscard]] size_t entity_count() const noexcept { return mEntityCount; }

	private:
		std::string mName;
		Registry mRegistry;
		size_t mEntityCount = 0;
	};
}