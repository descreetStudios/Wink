#pragma once

namespace Wink::GFX::RES
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

	struct MeshTag {};
	struct ShaderTag {};
	struct TextureTag {};
	struct CubemapTag {};
	struct MaterialTag {};
	struct ModelTag {};

	using MeshHandle = Handle<MeshTag>;
	using ShaderHandle = Handle<ShaderTag>;
	using TextureHandle = Handle<TextureTag>;
	using CubemapHandle = Handle<CubemapTag>;
	using MaterialHandle = Handle<MaterialTag>;
	using ModelHandle = Handle<ModelTag>;
}

template<typename Tag>
struct std::hash<Wink::GFX::RES::Handle<Tag>>
{
	size_t operator()(const Wink::GFX::RES::Handle<Tag>& h) const noexcept
	{
		const u64 packed = (static_cast<u64>(h.index) << 32)
			| static_cast<u64>(h.generation);

		return std::hash<u64>{}(packed);
	}
};