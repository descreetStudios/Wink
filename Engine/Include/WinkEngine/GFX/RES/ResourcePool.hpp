#pragma once

#include <WinkEngine/GFX/RES/Handle.hpp>

namespace Wink::GFX::RES
{
	class IResourcePool
	{
	public:
		virtual ~IResourcePool() = default;

		[[nodiscard]] virtual size_t get_live_count() const noexcept = 0;
		[[nodiscard]] virtual size_t get_capacity() const noexcept = 0;
		virtual void clear() noexcept = 0;
	};

	template<typename T, typename Tag>
	class ResourcePool : public IResourcePool
	{
	public:
		using HandleType = Handle<Tag>;

		ResourcePool() = default;
		~ResourcePool() override = default;

		DISABLE_COPY(ResourcePool);
		DISABLE_MOVE(ResourcePool);

		template<typename... Args>
		[[nodiscard]] HandleType allocate(Args&&... args)
		{
			const u32 index = acquire_slot();
			Slot& slot = mSlots[index];

			assert(!slot.value.has_value());
			slot.value.emplace(std::forward<Args>(args)...);

			return HandleType{ index, slot.generation };
		}

		void deallocate(HandleType handle) noexcept
		{
			if (!is_valid(handle)) return;

			Slot& slot = mSlots[handle.index];
			slot.value.reset();
			++slot.generation;
			mFreeList.push_back(handle.index);
			--mLiveCount;
		}

		[[nodiscard]] bool is_valid(HandleType handle) const noexcept
		{
			return handle.is_valid()
				&& handle.index < mSlots.size()
				&& mSlots[handle.index].generation == handle.generation
				&& mSlots[handle.index].value.has_value();
		}

		[[nodiscard]] T* try_get(HandleType handle) noexcept
		{
			if (!is_valid(handle)) return nullptr;
			return &*mSlots[handle.index].value;
		}

		[[nodiscard]] const T* try_get(HandleType handle) const noexcept
		{
			if (!is_valid(handle)) return nullptr;
			return &*mSlots[handle.index].value;
		}

		[[nodiscard]] size_t get_live_count() const noexcept override { return mLiveCount; }
		[[nodiscard]] size_t get_capacity() const noexcept override { return mSlots.size(); }

		void clear() noexcept override
		{
			for (Slot& slot : mSlots)
			{
				if (slot.value.has_value())
				{
					slot.value.reset();
					++slot.generation;
				}
			}

			mFreeList.clear();
			mFreeList.reserve(mSlots.size());
			for (u32 i = static_cast<u32>(mSlots.size()); i-- > 0; )
				mFreeList.push_back(i);

			mLiveCount = 0;
		}

	protected:
		template<typename Fn>
		bool with(HandleType handle, Fn&& fn)
		{
			if (!is_valid(handle)) return false;
			fn(*const_cast<T*>(&*mSlots[handle.index].value));
			return true;
		}

		template<typename Fn>
		bool with(HandleType handle, Fn&& fn) const
		{
			if (!is_valid(handle)) return false;
			fn(*const_cast<T*>(&*mSlots[handle.index].value));
			return true;
		}

		bool replace(HandleType handle, T&& newValue) noexcept
		{
			if (!is_valid(handle)) return false;
			mSlots[handle.index].value = std::move(newValue);
			return true;
		}

	private:
		struct Slot
		{
			std::optional<T> value;
			u32 generation = 0;
		};

		[[nodiscard]] u32 acquire_slot()
		{
			if (!mFreeList.empty())
			{
				const u32 index = mFreeList.back();
				mFreeList.pop_back();
				++mLiveCount;
				return index;
			}

			assert(mSlots.size() < HandleType::INVALID_INDEX);

			mSlots.emplace_back();
			++mLiveCount;
			return static_cast<u32>(mSlots.size() - 1);
		}

	private:
		std::deque<Slot> mSlots;
		std::vector<u32> mFreeList;
		size_t mLiveCount = 0;
	};
}