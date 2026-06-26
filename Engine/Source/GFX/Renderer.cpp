#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Renderer.hpp>

#include <WinkEngine/ECS/Scene.hpp>
#include <WinkEngine/ECS/Components/TransformComponent.hpp>
#include <WinkEngine/ECS/Components/RenderObjectComponent.hpp>
#include <WinkEngine/ECS/Components/CameraComponent.hpp>
#include <WinkEngine/ECS/Components/LightComponent.hpp>
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
		ShaderHandle gEquirectToCubemapShader;

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
			auto& meshPool = get_mesh_pool();

			auto material = matPool.is_valid(drawData.renderObj.material) ?
				drawData.renderObj.material : gDefaultMaterial;
			auto mesh = drawData.renderObj.mesh;
			auto* shader = get_shader_pool().
				try_get(matPool.try_get(material)->shader);

			matPool.apply(material);

			/* --- Core Uniforms --- */
			shader->set("uCamPos", drawData.camData.position);
			shader->set("uViewProj", drawData.camData.viewProj);
			shader->set("uModel", drawData.modelMat);
			shader->set("uNormalMatrix", drawData.normalMat);

			/* --- Dir Lights --- */
			i32 lightCount = drawData.dirLights.size();
			shader->set("uDirLightCount", lightCount);
			for (i32 i = 0; i < lightCount; ++i)
			{
				const auto& light = drawData.dirLights[i];
				const std::string base = "uDirLights[" + std::to_string(i) + "].";
				shader->set(base + "direction", light.direction);
				shader->set(base + "intensity", light.intensity);
				shader->set(base + "color", light.color);
			}

			/* --- Point Lights --- */
			lightCount = drawData.pointLights.size();
			shader->set("uPointLightCount", lightCount);
			for (i32 i = 0; i < lightCount; ++i)
			{
				const auto& light = drawData.pointLights[i];
				const std::string base = "uPointLights[" + std::to_string(i) + "].";
				shader->set(base + "position", light.position);
				shader->set(base + "intensity", light.intensity);
				shader->set(base + "color", light.color);
				shader->set(base + "radius", light.radius);
			}

			/* --- Spot Lights --- */
			lightCount = drawData.spotLights.size();
			shader->set("uSpotLightCount", lightCount);
			for (i32 i = 0; i < lightCount; ++i)
			{
				const auto& light = drawData.spotLights[i];
				const std::string base = "uSpotLights[" + std::to_string(i) + "].";
				shader->set(base + "position", light.position);
				shader->set(base + "range", light.range);
				shader->set(base + "direction", light.direction);
				shader->set(base + "innerCutoff", light.innerCutoff);
				shader->set(base + "color", light.color);
				shader->set(base + "outerCutoff", light.outerCutoff);
				shader->set(base + "intensity", light.intensity);
			}

			/* --- Draw --- */
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

		/* --- Scene --- */
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

		/* --- Camera --- */
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
			.viewProj = cam.get_proj() * cam.get_view() };

		/* --- Lights --- */
		std::vector<DirLight> dirLights;
		std::vector<PointLight> pointLights;
		std::vector<SpotLight> spotLights;
		dirLights.reserve(MAX_DIR_LIGHTS);
		pointLights.reserve(MAX_POINT_LIGHTS);
		spotLights.reserve(MAX_SPOT_LIGHTS);

		for (auto&& [id, dlC] :
			scene->view<ECS::DirLightComponent>())
		{
			if (static_cast<i32>(dirLights.size()) >= MAX_DIR_LIGHTS)
				break;

			dirLights.push_back(dlC.dirLight);
		}

		for (auto&& [id, plC] :
			scene->view<ECS::PointLightComponent>())
		{
			if (static_cast<i32>(pointLights.size()) >= MAX_POINT_LIGHTS)
				break;

			PointLight pl = plC.pointLight;

			auto e = scene->wrap(id);
			if (e.has<ECS::TransformComponent>())
				pl.position += e.get<ECS::TransformComponent>().position;

			pointLights.push_back(pl);
		}

		for (auto&& [id, slC] :
			scene->view<ECS::SpotLightComponent>())
		{
			if (static_cast<i32>(spotLights.size()) >= MAX_SPOT_LIGHTS)
				break;

			SpotLight sl = slC.spotLight;

			auto e = scene->wrap(id);
			if (e.has<ECS::TransformComponent>())
			{
				auto& t = e.get<ECS::TransformComponent>();
				sl.position += t.position;
			}

			spotLights.push_back(sl);
		}

		/* --- RenderObject --- */
		for (auto&& [id, tC, roC] :
			scene->view<ECS::TransformComponent,
			ECS::RenderObjectComponent>())
		{
			auto e = scene->wrap(id);
			if (tC.dirty)
				ECS::update_world_transform(*scene, id);

			draw({
				.renderObj = roC.renderObj,
				.camData = camData,
				.modelMat = tC.worldMatrix,
				.normalMat = glm::transpose(glm::inverse(glm::mat3(tC.worldMatrix))),
				.dirLights = dirLights, .pointLights = pointLights, .spotLights = spotLights });
		}
	}

	bool init()
	{
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			Logger::Internal::critical("Failed to initialize GLAD");
			return false;
		}

		if (!create_default_material())
		{
			Logger::Internal::error("Failed to create default material");
			return false;
		}

		gEquirectToCubemapShader = gShaderPool.load(std::vector<ShaderFile>{
			{ ShaderType::Compute, "Shaders/EquirectToCubemapCS.glsl" },
		});

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
		ShaderHandle get_equirect_to_cubemap_shader() noexcept { return gEquirectToCubemapShader; }

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