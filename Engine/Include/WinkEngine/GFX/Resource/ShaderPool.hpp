#pragma once

#include <WinkEngine/GFX/Resource/ResourcePool.hpp>
#include <WinkEngine/GFX/Shader.hpp>

namespace Wink::GFX::Resource
{
	class ShaderPool final : public ResourcePool<ShaderProgram, ShaderTag>
	{
	public:
		ShaderHandle load(const std::vector<ShaderSource>& sources);
		ShaderHandle load(const std::vector<ShaderFile>& files);
		void unload(ShaderHandle handle) noexcept;

		[[nodiscard]] u32 get_id(ShaderHandle handle) const noexcept;
		[[nodiscard]] bool is_valid(ShaderHandle handle) const noexcept;

	private:
		[[nodiscard]] ShaderHandle commit(
			std::optional<ShaderProgram> program);
	};
}