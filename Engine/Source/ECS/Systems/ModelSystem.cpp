#include <WinkEngine/pch.hpp>
#include <WinkEngine/ECS/Systems/ModelSystem.hpp>
#include <WinkEngine/ECS/Components/TransformComponent.hpp>
#include <WinkEngine/ECS/Components/RenderObjectComponent.hpp>
#include <WinkEngine/ECS/Systems/TransformSystem.hpp>
#include <WinkEngine/Content/Model.hpp>
#include <WinkEngine/GFX/Renderer.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::ECS
{
	EntityID instantiate_model(
		GFX::Resource::ModelHandle handle,
		Scene* scene, std::optional<EntityID> root)
	{
		using namespace GFX;
		using namespace Content;

		if (!scene)
		{
			scene = get_active_scene();
			if (!scene)
			{
				Logger::Internal::error(
					"Failed to find a valid scene to instantiate model");
				return NULL_ENTITY;
			}
		}

		const Model* model = Resource::get_model_pool().try_get(handle);
		if (!model)
		{
			Logger::Internal::error(
				"Trying to instantiate an invalid model");
			return NULL_ENTITY;
		}

		EntityID modelRoot;
		if (root.has_value()) modelRoot = *root;
		else
		{
			auto e = scene->spawn();
			e.add<TransformComponent>();
			modelRoot = e.get_id();
		}

		// First pass: spawn all node entities and set local transforms
		std::vector<ECS::EntityID> nodeEntities(model->nodes.size(), ECS::NULL_ENTITY);
		for (size_t i = 0; i < model->nodes.size(); ++i)
		{
			const ModelNode& node = model->nodes[i];
			auto e = scene->spawn();

			auto& transform = e.add<TransformComponent>();
			transform.position = node.position;
			transform.rotation = node.rotation;
			transform.scale = node.scale;

			nodeEntities[i] = e.get_id();
		}

		// Second pass: wire up parents and spawn primitives
		for (size_t i = 0; i < model->nodes.size(); ++i)
		{
			const ModelNode& node = model->nodes[i];

			const ECS::EntityID nodeEntity = nodeEntities[i];
			const ECS::EntityID parentEntity = node.parent.has_value()
				? nodeEntities[*node.parent]
				: modelRoot;

			attach_to_parent(*scene, nodeEntity, parentEntity);

			for (const ModelPrimitive& prim : node.primitives)
			{
				auto pe = scene->spawn();

				pe.add<TransformComponent>();
				attach_to_parent(*scene, pe.get_id(), nodeEntity);

				auto& renderObj = pe.add<RenderObjectComponent>();
				renderObj.renderObj.mesh = prim.mesh;
				renderObj.renderObj.material = prim.material;
			}
		}

		return modelRoot;
	}
}