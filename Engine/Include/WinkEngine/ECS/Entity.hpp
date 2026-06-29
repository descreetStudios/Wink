#pragma once

#include <WinkEngine/Core/Logger.hpp>

namespace Wink::ECS
{
	class Scene;

	using IDType = entt::id_type;
	using EntityID = entt::entity;
	using Registry = entt::registry;

	inline constexpr EntityID NULL_ENTITY = entt::null;

	template<typename T>
	concept Component = std::is_move_constructible_v<T>;

	class Entity
	{
	public:
		Entity() = default;
		Entity(EntityID id, Registry& reg) noexcept;
		Entity(IDType id, Registry& reg) noexcept;
		Entity(EntityID id, Scene& scene) noexcept;
		Entity(IDType id, Scene& scene) noexcept;

		template <Component C, typename... Args>
		C& add(Args&&... args)
		{
			assert(is_valid());
			if (mReg->any_of<C>(mID))
			{
				Logger::Internal::warn("Trying to add double component to an entity");
				return mReg->get<C>(mID);
			}
			return mReg->emplace<C>(mID, std::forward<Args>(args)...);
		}

		template <Component C, typename... Args>
		C& set(Args&&... args)
		{
			assert(is_valid());
			return mReg->emplace_or_replace<C>(mID, std::forward<Args>(args)...);
		}

		template <Component C>
		Entity& remove()
		{
			assert(is_valid());
			mReg->remove<C>(mID);
			return *this;
		}

		/* Component access */
		template <Component C> [[nodiscard]] C& get() { assert(is_valid()); return mReg->get<C>(mID); }
		template <Component C> [[nodiscard]] const C& get() const { assert(is_valid()); return mReg->get<C>(mID); }

		template <Component C> [[nodiscard]] C* try_get() noexcept { return mReg ? mReg->try_get<C>(mID) : nullptr; }
		template <Component C> [[nodiscard]] const C* try_get() const noexcept { return mReg ? mReg->try_get<C>(mID) : nullptr; }

		template <Component C> [[nodiscard]] bool has() const noexcept { return mReg && mReg->all_of<C>(mID); }
		template <Component... Cs> [[nodiscard]] bool has_all() const noexcept { return mReg && mReg->all_of<Cs...>(mID); }
		template <Component... Cs> [[nodiscard]] bool has_any() const noexcept { return mReg && mReg->any_of<Cs...>(mID); }

		[[nodiscard]] EntityID get_id() const noexcept { return mID; }
		[[nodiscard]] const Registry* get_registry() const noexcept { return mReg; }
		[[nodiscard]] bool is_valid() const noexcept { return mReg && mReg->valid(mID); }
		operator bool() const noexcept { return is_valid(); }

		bool operator==(const Entity& o) const noexcept { return mID == o.mID; }
		bool operator!=(const Entity& o) const noexcept { return mID != o.mID; }

		operator IDType() const noexcept { return static_cast<IDType>(mID); }
		operator EntityID() const noexcept { return mID; }

		// Call this function from a scene, so the entities counter gets correctly updated
		void destroy();

	private:
		EntityID mID = NULL_ENTITY;
		Registry* mReg = nullptr;
	};
}