#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/RES/ModelPool.hpp>
#include <WinkEngine/GFX/Renderer.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::GFX::RES
{
	ModelHandle ModelPool::load(
		const fs::path& path, ShaderHandle shader)
	{
		fs::path filePath;

		if (fs::is_regular_file(path)) filePath = path;
		else if (fs::is_directory(fs::absolute(path)))
		{
			for (const auto& entry : fs::recursive_directory_iterator(path))
			{
				if (!entry.is_regular_file())
					continue;

				const auto extension = entry.path().extension().string();

				for (const auto& format :
					Content::Internal::SUPPORTED_MODEL_FORMATS)
				{
					if (extension == format)
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
					path.string()
				);

				return ModelHandle();
			}
		}
		else
		{
			Logger::Internal::error(
				"Invalid model path '{}'",
				path.string()
			);

			return ModelHandle();
		}

		const auto extension = filePath.extension().string();

		bool supported = false;
		for (const auto& format :
			Content::Internal::SUPPORTED_MODEL_FORMATS)
		{
			if (extension == format)
			{
				supported = true;
				break;
			}
		}

		if (!supported)
		{
			Logger::Internal::error(
				"Unsupported model format '{}'",
				extension
			);

			return ModelHandle();
		}

		std::optional<Content::Model> model;
		if (extension == ".gltf" || extension == ".glb")
			model = Content::Internal::load_gltf(filePath, shader);

		if (!model.has_value())
			return GFX::RES::ModelHandle{};

		return allocate(std::move(*model));
	}

	void ModelPool::unload(ModelHandle handle) noexcept
	{
		deallocate(handle);
	}

	bool ModelPool::is_valid(ModelHandle handle) const noexcept
	{
		bool valid = ResourcePool::is_valid(handle);
		with(handle, [&](Content::Model& model) {
			if (!valid) return false;
			for (size_t i = 0; i < model.nodes.size(); ++i)
			{
				auto& node = model.nodes[i];
				for (size_t j = 0; j < node.primitives.size(); ++j)
				{
					auto& prim = node.primitives[j];
					if (!GFX::RES::get_mesh_pool().is_valid(prim.mesh) ||
						!GFX::RES::get_material_pool().is_valid(prim.material))
					{
						valid = false;
						break;
					}
				}
			}
			return true;
			});
		return valid;
	}
}