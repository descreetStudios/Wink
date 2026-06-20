#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Resource/ShaderPool.hpp>

namespace Wink::GFX::Resource
{
	ShaderHandle ShaderPool::load(const std::vector<ShaderSource>& sources)
	{
		return commit(Internal::create_program(sources));
	}

	ShaderHandle ShaderPool::load(const std::vector<ShaderFile>& files)
	{
		return commit(Internal::create_program(files));
	}

	void ShaderPool::unload(ShaderHandle handle) noexcept
	{
		deallocate(handle);
	}

	u32 ShaderPool::get_id(ShaderHandle handle) const noexcept
	{
		u32 id = 0;
		with(handle, [&](ShaderProgram& p) { id = p.get_id(); });
		return id;
	}

	bool ShaderPool::is_valid(ShaderHandle handle) const noexcept
	{
		bool valid = ResourcePool::is_valid(handle);
		with(handle, [&](ShaderProgram& p) { valid &= p.is_valid(); });
		return valid;
	}

	ShaderHandle ShaderPool::commit(
		std::optional<ShaderProgram> program)
	{
		if (!program.has_value())
			return ShaderHandle();

		return allocate(std::move(*program));
	}
}