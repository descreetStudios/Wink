#pragma once

#include <WinkEngine/Resource/ResourcePool.hpp>
#include <WinkEngine/GFX/Shader.hpp>

namespace Wink
{
	namespace Resource
	{
		struct ShaderTag;

		class ShaderPool final : public ResourcePool<GFX::ShaderProgram, ShaderTag>
		{
		public:
			ShaderHandle load(const std::vector<GFX::ShaderSource>& sources);
			ShaderHandle load(const std::vector<GFX::ShaderFile>& files);
			void unload(ShaderHandle handle) noexcept;

			/* Shader operations */
			void use(ShaderHandle handle) const noexcept;

			template <typename ValueT>
			void set(ShaderHandle handle, std::string_view name,
				const ValueT& value) const noexcept
			{
				with(handle, [&](GFX::ShaderProgram& p) {
					p.set(name, value); });
			}

			void set_texture(ShaderHandle handle,
				std::string_view name, i32 unit) const noexcept;
			void bind_ubo_block(ShaderHandle handle,
				std::string_view blockName, u32 bindingPoint) const noexcept;

			[[nodiscard]] u32 get_id(ShaderHandle handle) const noexcept;

		private:
			[[nodiscard]] ShaderHandle commit(
				std::optional<GFX::ShaderProgram> program);
		};
	}
}