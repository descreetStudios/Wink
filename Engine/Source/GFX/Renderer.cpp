#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Renderer.hpp>

#include <WinkEngine/ECS/Scene.hpp>
#include <WinkEngine/ECS/Components/TransformComponent.hpp>
#include <WinkEngine/ECS/Components/RenderObjectComponent.hpp>
#include <WinkEngine/ECS/Components/CameraComponent.hpp>
#include <WinkEngine/ECS/Systems/TransformSystem.hpp>

#include <WinkEngine/Core/Logger.hpp>
#include <GLFW/glfw3.h>

namespace Wink::GFX
{
	namespace
	{
		using namespace Resource;

		MeshPool gMeshPool;
		ShaderPool gShaderPool;
		TexturePool gTexturePool;
		MaterialPool gMaterialPool;
		ModelPool gModelPool;

		MaterialHandle gDefaultMaterial;
		ShaderHandle gDefaultShader;

		bool create_default_material()
		{
			gDefaultShader = gShaderPool.load(std::vector<ShaderFile>{
				{ ShaderType::Vertex, "Shaders/default_vs.glsl" },
				{ ShaderType::Fragment, "Shaders/default_fs.glsl" },
			});

			gDefaultMaterial = gMaterialPool.create(gDefaultShader);

			return gMaterialPool.is_valid(gDefaultMaterial);
		}

		void draw(const DrawData& drawData)
		{
			auto& matPool = get_material_pool();
			auto& shaderPool = get_shader_pool();
			auto& meshPool = get_mesh_pool();

			auto material = matPool.is_valid(drawData.renderObj.material) ?
				drawData.renderObj.material : gDefaultMaterial;

			auto mesh = drawData.renderObj.mesh;
			if (!meshPool.is_valid(mesh))
			{
				Logger::Internal::error("Trying to draw an invalid mesh");
				return;
			}

			//Logger::Internal::info("Draw call. RenderObject valid: '{}'\n"
			//	"Model Matrix: '{}'\nCamera Position: '{}'\n"
			//	"Camera View: '{}'\nCamera Proj: '{}'",
			//	matPool.is_valid(material) && meshPool.is_valid(mesh),
			//	glm::to_string(drawData.modelMat),
			//	glm::to_string(drawData.camData.position),
			//	glm::to_string(drawData.camData.view),
			//	glm::to_string(drawData.camData.proj));

			matPool.apply(material);
			auto* shader = shaderPool.try_get(matPool.try_get(material)->shader);

			// TODO: Set uniforms
			shader->set("uCamPos", drawData.camData.position);
			shader->set("uView", drawData.camData.view);
			shader->set("uProj", drawData.camData.proj);
			shader->set("uModel", drawData.modelMat);

			glBindVertexArray(meshPool.get_vao_id(mesh));
			glDrawElements(GL_TRIANGLES,
				static_cast<GLsizei>(meshPool.get_index_count(mesh)),
				GL_UNSIGNED_INT, nullptr);
		}
	} // anonymous namespace

	void render()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		auto scene = ECS::get_active_scene();
		static bool sceneWarn = true;
		if (!scene)
		{
			if (sceneWarn)
			{
				Logger::Internal::warn("No scene found to render from");
				sceneWarn = false;
			}
			return;
		}

		auto camEOpt = scene->find_first<ECS::CameraComponent>();
		static bool camWarn = true;
		if (!camEOpt)
		{
			if (camWarn)
			{
				Logger::Internal::warn("No camera component found in scene");
				camWarn = false;
			}
			return;
		}

		auto camE = *camEOpt;
		Camera cam = camE.get<ECS::CameraComponent>().camera; // intended copy

		if (camE.has<ECS::TransformComponent>())
		{
			auto& camET = camE.get<ECS::TransformComponent>();
			cam.position += camET.position;
		}

		CameraData camData{ .position = cam.position,
			.view = cam.get_view(), .proj = cam.get_proj() };

		for (auto&& [id, tC, roC] :
			scene->view<ECS::TransformComponent,
			ECS::RenderObjectComponent>())
		{
			auto e = scene->wrap(id);
			if (tC.dirty)
				ECS::update_world_transform(*scene, id);

			draw({ .renderObj=roC.renderObj,
				.camData = camData,
				.modelMat = tC.worldMatrix});
		}
	}

	bool init()
	{
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			Logger::Internal::critical("Failed to initialize GLAD");
			return false;
		}

		// TODO: Make a GFX::Configuration,
		// store every renderer config and init with them
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		if (!create_default_material())
		{
			Logger::Internal::error("Failed to create default material");
			return false;
		}

		return true;
	}

	void shutdown()
	{
		Resource::clear_all_resources();
	}

	void resize(u32 width, u32 height)
	{
		glViewport(0, 0,
			static_cast<GLsizei>(width),
			static_cast<GLsizei>(height));
	}

	void set_clear_color(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	namespace Resource
	{
		MeshPool& get_mesh_pool() noexcept { return gMeshPool; }
		ShaderPool& get_shader_pool() noexcept { return gShaderPool; }
		TexturePool& get_texture_pool() noexcept { return gTexturePool; }
		MaterialPool& get_material_pool() noexcept { return gMaterialPool; }
		ModelPool& get_model_pool() noexcept { return gModelPool; }

		ShaderHandle get_default_shader() noexcept { return gDefaultShader; }
		MaterialHandle get_default_material() noexcept { return gDefaultMaterial; }

		void clear_all_resources() noexcept
		{
			gShaderPool.clear();
			gTexturePool.clear();
		}

		void poll_hot_reloads() noexcept
		{
			gShaderPool.poll_hot_reload();
			gTexturePool.poll_hot_reload();
		}
	}
}