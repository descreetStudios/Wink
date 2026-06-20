#include <WinkEngine/pch.hpp>
#include <WinkEngine/Resource/ShaderPool.hpp>
#include <WinkEngine/GFX/Shader.hpp>

namespace Wink::Resource
{
	ShaderHandle ShaderPool::load(const std::vector<GFX::ShaderSource>& sources)
	{
		return commit(GFX::Internal::create_program(sources));
	}

	ShaderHandle ShaderPool::load(const std::vector<GFX::ShaderFile>& files)
	{
		return commit(GFX::Internal::create_program(files));
	}

	void ShaderPool::unload(ShaderHandle handle) noexcept
	{
		deallocate(handle);
	}

	void ShaderPool::use(ShaderHandle handle) const noexcept
	{
		with(handle, [](GFX::ShaderProgram& p) { p.use(); });
	}

	void ShaderPool::set_texture(ShaderHandle handle,
		std::string_view name, i32 unit) const noexcept
	{
		with(handle, [&](GFX::ShaderProgram& p) { p.set_texture(name, unit); });
	}

	void ShaderPool::bind_ubo_block(ShaderHandle handle,
		std::string_view blockName, u32 bindingPoint) const noexcept
	{
		with(handle, [&](GFX::ShaderProgram& p) {
			p.bind_ubo_block(blockName, bindingPoint); });
	}

	u32 ShaderPool::get_id(ShaderHandle handle) const noexcept
	{
		u32 id = 0;
		with(handle, [&](GFX::ShaderProgram& p) { id = p.get_id(); });
		return id;
	}

	ShaderHandle ShaderPool::commit(
		std::optional<GFX::ShaderProgram> program)
	{
		if (!program.has_value())
			return ShaderHandle();

		return allocate(std::move(*program));
	}
}