#pragma once

#include <WinkEngine/GFX/Resource/ResourcePool.hpp>
#include <WinkEngine/GFX/Shader.hpp>
#include <WinkEngine/Core/FileWatcher.hpp>

namespace Wink::GFX::Resource
{
	class ShaderPool final : public ResourcePool<ShaderProgram, ShaderTag>
	{
	public:
		ShaderHandle load(const std::vector<ShaderSource>& sources);
		ShaderHandle load(const std::vector<ShaderFile>& files, bool hotReload = true);
		void unload(ShaderHandle handle) noexcept;

		void set_hot_reload(ShaderHandle handle, bool enabled) const noexcept;
		void poll_hot_reload();

		[[nodiscard]] u32 get_id(ShaderHandle handle) const noexcept;
		[[nodiscard]] bool is_valid(ShaderHandle handle) const noexcept;

	private:
		[[nodiscard]] ShaderHandle commit(
			std::optional<ShaderProgram> program);

		void start_watching(ShaderHandle handle) const;
		void stop_watching(ShaderHandle handle) const;
		void reload(ShaderHandle handle) const;

	private:
		std::unordered_map<u32, std::vector<ShaderFile>> mReloadSources;
		mutable FileWatcher mWatcher;
	};
}