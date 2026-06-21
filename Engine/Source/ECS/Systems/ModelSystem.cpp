#include <WinkEngine/pch.hpp>
#include <WinkEngine/ECS/Systems/ModelSystem.hpp>
#include <WinkEngine/ECS/Components/TransformComponent.hpp>
#include <WinkEngine/ECS/Components/RenderObjectComponent.hpp>
#include <WinkEngine/Content/Model.hpp>
#include <WinkEngine/GFX/Renderer.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::ECS
{
	namespace
	{
		EntityID create_node_entity(Scene& scene,
			const Content::ModelNode& node, EntityID parent)
		{
			auto e = scene.spawn();

			auto& transform = e.add<TransformComponent>();
			transform.position = node.position;
			transform.rotation = node.rotation;
			transform.scale = node.scale;
			transform.parent = parent;

			return e.get_id();
		}
	} // anonymous namespace

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

		const EntityID modelRoot = root.value_or(scene->spawn());
		if (!root.has_value())
			scene->wrap(modelRoot).add<TransformComponent>();

		std::vector<ECS::EntityID> nodeEntities(model->nodes.size(), ECS::NULL_ENTITY);
		for (size_t i = 0; i < model->nodes.size(); ++i)
		{
			const ModelNode& node = model->nodes[i];

			const ECS::EntityID parent = node.parent.has_value()
				? nodeEntities[*node.parent]
				: modelRoot;

			const ECS::EntityID nodeEntity = create_node_entity(*scene, node, parent);
			nodeEntities[i] = nodeEntity;

			for (const ModelPrimitive& prim : node.primitives)
			{
				const ECS::EntityID primEntity = scene->spawn();
				auto pe = scene->wrap(primEntity);

				auto& primTransform = pe.add<ECS::TransformComponent>();
				primTransform.parent = nodeEntity;

				auto& renderObj = pe.add<RenderObjectComponent>();
				renderObj.renderObj.mesh = prim.mesh;
				renderObj.renderObj.material = prim.material;
			}
		}

		return modelRoot;
	}
}