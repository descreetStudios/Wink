#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/RES/ModelPool.hpp>
#include <WinkEngine/GFX/Renderer.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::GFX::RES
{
	ModelHandle ModelPool::load(const fs::path& path, ShaderHandle shader)
	{
		fs::path filePath;

		if (fs::is_regular_file(path)) filePath = path;
		else if (fs::is_directory(fs::absolute(path)))
		{
			for (const auto& entry : fs::recursive_directory_iterator(path))
			{
				if (!entry.is_regular_file())
					continue;

				const auto ext = entry.path().extension().string();

				for (const auto& format : Content::Internal::SUPPORTED_MODEL_FORMATS)
				{
					if (ext == format)
					{
						filePath = entry.path();
						break;
					}
				}

				if (!filePath.empty())
					break;
			}

			if (filePath.empty())
			{
				Logger::Internal::error(
					"No supported model format found in folder '{}'",
					path.string());
				return {};
			}
		}
		else
		{
			Logger::Internal::error(
				"Invalid model path '{}'",
				path.string());
			return {};
		}

		const auto ext = filePath.extension().string();

		bool supported = false;
		for (const auto& format : Content::Internal::SUPPORTED_MODEL_FORMATS)
		{
			if (ext == format)
			{
				supported = true;
				break;
			}
		}

		if (!supported)
		{
			Logger::Internal::error(
				"Unsupported model format '{}'", ext);
			return {};
		}

		std::optional<Content::Model> model;
		if (ext == ".gltf" || ext == ".glb")
			model = Content::Internal::load_gltf(filePath, shader);

		if (!model.has_value()) return {};

		return allocate(std::move(*model));
	}

	void ModelPool::unload(ModelHandle handle) noexcept
	{
		deallocate(handle);
	}

	bool ModelPool::is_valid(ModelHandle handle) const noexcept
	{
		if (!ResourcePool::is_valid(handle))
			return false;

		return get_or(handle, [](Content::Model& model) -> bool
			{
				auto& meshPool = GFX::RES::get_mesh_pool();
				auto& materialPool = GFX::RES::get_material_pool();

				for (const auto& node : model.nodes)
				{
					for (const auto& prim : node.primitives)
					{
						if (!meshPool.is_valid(prim.mesh) ||
							!materialPool.is_valid(prim.material))
							return false;
					}
				}

				return true;
			});
	}
}