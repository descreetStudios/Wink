#pragma once

namespace Wink::Resource
{
	template<typename Tag>
	struct Handle
	{
		static constexpr u32 INVALID_INDEX = std::numeric_limits<u32>::max();

		u32 index = INVALID_INDEX;
		u32 generation = 0;

		[[nodiscard]] constexpr bool is_valid() const noexcept
		{
			return index != INVALID_INDEX;
		}

		constexpr explicit operator bool() const noexcept { return is_valid(); }

		[[nodiscard]] friend constexpr bool operator==(
			const Handle&, const Handle&) noexcept = default;
	};

	struct ShaderTag {};

	using ShaderHandle = Handle<ShaderTag>;
}

template<typename Tag>
struct std::hash<Wink::Resource::Handle<Tag>>
{
	size_t operator()(const Wink::Resource::Handle<Tag>& h) const noexcept
	{
		return (static_cast<size_t>(h.index) << 32) ^ h.generation;
	}
};