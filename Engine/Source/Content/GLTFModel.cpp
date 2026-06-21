#include <WinkEngine/pch.hpp>
#include <WinkEngine/Content/Model.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::Content
{
	namespace
	{
	} // anonymous namespace

	namespace Internal
	{
		std::optional<Model> load_gltf(const fs::path& path,
			GFX::Resource::ShaderHandle shader)
		{
			Logger::Internal::info("Reached load_gltf(), path: '{}'",
				fs::absolute(path).string());
			return std::nullopt;
		}
	}
}