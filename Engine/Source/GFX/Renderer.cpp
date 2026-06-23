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

		Configuration gConfig;

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
				{ ShaderType::Vertex, "Shaders/DefaultVS.glsl" },
				{ ShaderType::Fragment, "Shaders/DefaultFS.glsl" },
			});

			gDefaultMaterial = gMaterialPool.create(gDefaultShader);

			return gMaterialPool.is_valid(gDefaultMaterial);
		}

		void apply_config(const Configuration& cfg)
		{
			/* --- Depth --- */
			if (cfg.depthTest) glEnable(GL_DEPTH_TEST);
			else glDisable(GL_DEPTH_TEST);

			glDepthMask(cfg.depthWrite ? GL_TRUE : GL_FALSE);
			glDepthFunc(cfg.depthFunc);

			/* --- Stencil --- */
			if (cfg.stencilTest) glEnable(GL_STENCIL_TEST);
			else glDisable(GL_STENCIL_TEST);

			if (cfg.stencilTest)
			{
				glStencilFunc(cfg.stencilFunc,
					cfg.stencilRef, cfg.stencilMask);
				glStencilOp(cfg.stencilOpSfail,
					cfg.stencilOpDpfail, cfg.stencilOpDppass);
			}

			/* --- Blending --- */
			if (cfg.blend) glEnable(GL_BLEND);
			else glDisable(GL_BLEND);

			if (cfg.blend)
			{
				glBlendFunc(cfg.blendSrc, cfg.blendDst);
				glBlendEquation(cfg.blendEq);
			}

			/* --- Face culling --- */
			if (cfg.cullFace) glEnable(GL_CULL_FACE);
			else glDisable(GL_CULL_FACE);

			if (cfg.cullFace)
			{
				glCullFace(cfg.cullMode);
				glFrontFace(cfg.frontFace);
			}

			/* --- Polygon mode --- */
			glPolygonMode(GL_FRONT_AND_BACK, cfg.polygonMode);

			/* --- Multisampling --- */
			if (cfg.multisample) glEnable(GL_MULTISAMPLE);
			else glDisable(GL_MULTISAMPLE);
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

			matPool.apply(material);
			auto* shader = shaderPool.try_get(matPool.try_get(material)->shader);

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
		apply_config(gConfig);

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

			draw({ .renderObj = roC.renderObj,
				.camData = camData,
				.modelMat = tC.worldMatrix });
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

	void set_config(const Configuration& cfg)
	{
		gConfig = cfg;
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