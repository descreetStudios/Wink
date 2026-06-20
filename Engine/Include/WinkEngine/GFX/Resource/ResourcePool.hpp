#pragma once

#include <WinkEngine/GFX/Resource/Handle.hpp>

namespace Wink::GFX::Resource
{
	class IResourcePool
	{
	public:
		virtual ~IResourcePool() = default;

		[[nodiscard]] virtual size_t live_count() const noexcept = 0;
		[[nodiscard]] virtual size_t capacity() const noexcept = 0;
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

		[[nodiscard]] size_t live_count() const noexcept override { return mLiveCount; }
		[[nodiscard]] size_t capacity() const noexcept override { return mSlots.size(); }

		void clear() noexcept override
		{
			for (u32 i = 0; i < mSlots.size(); ++i)
			{
				Slot& slot = mSlots[i];
				if (slot.value.has_value())
				{
					slot.value.reset();
					++slot.generation;
				}
			}

			mFreeList.clear();
			for (u32 i = 0; i < mSlots.size(); ++i)
				mFreeList.push_back(mSlots.size() - 1 - i);
			mLiveCount = 0;
		}

	protected:
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