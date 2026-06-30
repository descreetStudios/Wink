#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/RES/ShaderPool.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::GFX::RES
{
	ShaderHandle ShaderPool::load(const std::vector<ShaderSource>& sources)
	{
		return commit(Internal::create_program(sources));
	}

	ShaderHandle ShaderPool::load(const std::vector<ShaderFile>& files, bool hotReload)
	{
		ShaderHandle handle = commit(Internal::create_program(files));
		if (!handle.is_valid())
			return handle;

		mReloadSources[handle.index] = files;
		set_hot_reload(handle, hotReload);
		return handle;
	}

	void ShaderPool::unload(ShaderHandle handle) noexcept
	{
		stop_watching(handle);
		mReloadSources.erase(handle.index);
		deallocate(handle);
	}

	void ShaderPool::set_hot_reload(
		ShaderHandle handle, bool enabled) const noexcept
	{
		with(handle, [&](ShaderProgram& p) { p.hotReloadEnabled = enabled; });

		if (enabled) start_watching(handle);
		else stop_watching(handle);
	}

	void ShaderPool::poll_hot_reload()
	{
		mWatcher.poll();
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

	void ShaderPool::start_watching(ShaderHandle handle) const
	{
		auto it = mReloadSources.find(handle.index);
		if (it == mReloadSources.end()) return;

		for (const ShaderFile& file : it->second)
		{
			mWatcher.watch(file.path, [this, handle](const fs::path&)
				{
					reload(handle);
				});
		}
	}

	void ShaderPool::stop_watching(ShaderHandle handle) const
	{
		auto it = mReloadSources.find(handle.index);
		if (it == mReloadSources.end()) return;

		for (const ShaderFile& file : it->second)
			mWatcher.unwatch(file.path);
	}

	void ShaderPool::reload(ShaderHandle handle) const
	{
		bool enabled = false;
		with(handle, [&](ShaderProgram& p) { enabled = p.hotReloadEnabled; });
		if (!enabled) return;

		Logger::Internal::trace("Hot-reloading shader");

		auto it = mReloadSources.find(handle.index);
		if (it == mReloadSources.end()) return;

		std::optional<ShaderProgram> rebuilt = Internal::create_program(it->second);
		if (!rebuilt.has_value())
		{
			Logger::Internal::error(
				"Shader hot-reload failed for handle index '{}'; keeping previous program.",
				handle.index);
			return;
		}

		rebuilt->hotReloadEnabled = true;
		const_cast<ShaderPool*>(this)->replace(handle, std::move(*rebuilt));
	}
}