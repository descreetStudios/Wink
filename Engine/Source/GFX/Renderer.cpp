#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Renderer.hpp>
#include <WinkEngine/GFX/Renderer/Data.hpp>
#include <WinkEngine/GFX/Renderer/GPUData.hpp>
#include <WinkEngine/GFX/Renderer/Pipeline.hpp>

#include <WinkEngine/GFX/Renderer/ForwardPlus/DepthPrePass.hpp>
#include <WinkEngine/GFX/Renderer/ForwardPlus/LightCullingPass.hpp>

#include <WinkEngine/ECS/Scene.hpp>
#include <WinkEngine/ECS/Components/TransformComponent.hpp>
#include <WinkEngine/ECS/Components/RenderObjectComponent.hpp>
#include <WinkEngine/ECS/Components/CameraComponent.hpp>
#include <WinkEngine/ECS/Components/LightComponent.hpp>
#include <WinkEngine/ECS/Components/IBLComponent.hpp>
#include <WinkEngine/ECS/Systems/TransformSystem.hpp>

#include <WinkEngine/Core/Window.hpp>
#include <GLFW/glfw3.h>

namespace Wink::GFX
{
	using namespace RES;

	/* --- Resource Pools --- */
	MeshPool gMeshPool;
	ShaderPool gShaderPool;
	TexturePool gTexturePool;
	CubemapPool gCubemapPool;
	MaterialPool gMaterialPool;
	ModelPool gModelPool;

	/* --- Default Material --- */
	ShaderHandle gDefaultShader;
	TextureHandle gDefaultAlbedo;
	TextureHandle gDefaultNormal;
	TextureHandle gDefaultMR;
	TextureHandle gDefaultAO;
	TextureHandle gDefaultEmissive;
	MaterialHandle gDefaultMaterial;

	/* --- UBOs --- */
	u32 gFrameUBO = 0;
	u32 gLightsUBO = 0;

	/* --- Fullscreen Triangle --- */
	ShaderHandle gFullscreenShader;
	MaterialHandle gFullscreenMaterial;
	u32 gFullscreenVAO = 0;

	/* --- Skybox --- */
	ShaderHandle gSkyboxShader;
	u32 gSkyboxVAO = 0;
	u32 gSkyboxVBO = 0;

	namespace ForwardPlus
	{
		ShaderHandle gDepthOnlyShader; ShaderHandle gDepthDebugShader;
		ShaderHandle gLightCullingShader; ShaderHandle gLightHeatmapShader;

		u32 gWidth;
		u32 gHeight;

		std::optional<DepthPrePass> gDepthPrePass;
		std::optional<LightCullingPass> gLightCullingPass;
	}

	namespace IBL
	{
		ShaderHandle gEquirectToCubemapShader;
		ShaderHandle gIrradianceConvolutionShader;
		ShaderHandle gPrefilteredEnvMapShader;
		TextureHandle gBRDFLUT;

		CubemapHandle gDefaultIrradiance;
		CubemapHandle gDefaultPrefiltered;

		IBLData gIBLData;
		bool gRenderSkybox;
	}

	namespace
	{
		constexpr float SKYBOX_VERTICES[]
		{
			// -X face
			-1.0f,  1.0f, -1.0f,   -1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f,  1.0f, -1.0f,
			// +X face
			1.0f, -1.0f, -1.0f,     1.0f, -1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,     1.0f,  1.0f, -1.0f,    1.0f, -1.0f, -1.0f,
			// -Y face
			-1.0f, -1.0f,  1.0f,   -1.0f, -1.0f, -1.0f,    1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,     1.0f, -1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,
			// +Y face
			-1.0f,  1.0f, -1.0f,   -1.0f,  1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,     1.0f,  1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,
			// +Z face
			-1.0f, -1.0f,  1.0f,    1.0f, -1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,    -1.0f,  1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,
			// -Z face
			-1.0f,  1.0f, -1.0f,    1.0f,  1.0f, -1.0f,    1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,    -1.0f, -1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,
		};

		void create_skybox_geometry()
		{
			glCreateVertexArrays(1, &gSkyboxVAO);
			glCreateBuffers(1, &gSkyboxVBO);

			glNamedBufferStorage(gSkyboxVBO,
				sizeof(SKYBOX_VERTICES), SKYBOX_VERTICES, 0);

			glVertexArrayVertexBuffer(gSkyboxVAO, 0, gSkyboxVBO, 0, 3 * sizeof(float));
			glEnableVertexArrayAttrib(gSkyboxVAO, 0);
			glVertexArrayAttribFormat(gSkyboxVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
			glVertexArrayAttribBinding(gSkyboxVAO, 0, 0);
		}

		TextureHandle create_1x1_texture(u8 r, u8 g, u8 b, u8 a)
		{
			Texture2DParams params;
			params.wrapS = TextureWrap::ClampToEdge;
			params.wrapT = TextureWrap::ClampToEdge;
			params.minFilter = TextureFilter::Nearest;
			params.magFilter = TextureFilter::Nearest;
			params.sRGB = false;
			params.hasAlpha = true;
			params.genMips = false;

			const u8 pixel[4]{ r, g, b, a };
			return gTexturePool.load(pixel, 1, 1, 4, TextureDataType::UnsignedByte, params);
		}

		bool load_engine_resources()
		{
			/* --- Default Material --- */
			gDefaultShader = gShaderPool.load(std::vector<ShaderFile>{
				{ ShaderType::Vertex, "Resources/Shaders/DefaultVS.glsl" },
				{ ShaderType::Fragment, "Resources/Shaders/DefaultFS.glsl" },
			});
			gDefaultAlbedo = create_1x1_texture(255, 255, 255, 255);
			gDefaultNormal = create_1x1_texture(128, 128, 255, 255);
			gDefaultMR = create_1x1_texture(0, 255, 255, 255);
			gDefaultAO = create_1x1_texture(255, 255, 255, 255);
			gDefaultEmissive = create_1x1_texture(255, 255, 255, 255);
			gDefaultMaterial = gMaterialPool.create(gDefaultShader);

			/* --- UBOs and SSBOs --- */
			glCreateBuffers(1, &gFrameUBO);
			glNamedBufferStorage(gFrameUBO, sizeof(FrameGPUData), nullptr,
				GL_DYNAMIC_STORAGE_BIT);

			glCreateBuffers(1, &gLightsUBO);
			glNamedBufferStorage(gLightsUBO, sizeof(LightsGPUData), nullptr,
				GL_DYNAMIC_STORAGE_BIT);

			Material::init_ssbo();

			/* --- Fullscreen Triangle --- */
			gFullscreenShader = gShaderPool.load(std::vector<ShaderFile>{
				{ ShaderType::Vertex, "Resources/Shaders/FullscreenVS.glsl" },
				{ ShaderType::Fragment, "Resources/Shaders/FullscreenFS.glsl" },
			});
			gFullscreenMaterial = gMaterialPool.create(gFullscreenShader);
			glCreateVertexArrays(1, &gFullscreenVAO);

			/* --- Skybox --- */
			gSkyboxShader = gShaderPool.load(std::vector<ShaderFile>{
				{ ShaderType::Vertex, "Resources/Shaders/SkyboxVS.glsl" },
				{ ShaderType::Fragment, "Resources/Shaders/SkyboxFS.glsl" },
			});

			create_skybox_geometry();

			/* --- Forward+ --- */
			ForwardPlus::gDepthOnlyShader = gShaderPool.load(std::vector<ShaderFile>{
				{ ShaderType::Vertex, "Resources/Shaders/ForwardPlus/DepthOnlyVS.glsl" },
				{ ShaderType::Fragment, "Resources/Shaders/ForwardPlus/DepthOnlyFS.glsl" },
			});

			ForwardPlus::gDepthDebugShader = gShaderPool.load(std::vector<ShaderFile>{
				{ ShaderType::Vertex, "Resources/Shaders/ForwardPlus/DepthDebugVS.glsl" },
				{ ShaderType::Fragment, "Resources/Shaders/ForwardPlus/DepthDebugFS.glsl" },
			});

			ForwardPlus::gLightCullingShader = gShaderPool.load(std::vector<ShaderFile>{
				//{ ShaderType::Compute, "Resources/Shaders/ForwardPlus/LightCullingCS.glsl" }
				{ ShaderType::Compute, "../../../../Engine/Source/GFX/Shaders/ForwardPlus/LightCullingCS.glsl" }
			});

			ForwardPlus::gLightHeatmapShader = gShaderPool.load(std::vector<ShaderFile>{
				{ ShaderType::Vertex, "Resources/Shaders/ForwardPlus/LightHeatmapVS.glsl" },
				{ ShaderType::Fragment, "Resources/Shaders/ForwardPlus/LightHeatmapFS.glsl" },
			});

			/* --- IBL --- */
			IBL::gEquirectToCubemapShader = gShaderPool.load(std::vector<ShaderFile>{
				{ ShaderType::Compute, "Resources/Shaders/EquirectToCubemapCS.glsl" }
			});

			IBL::gIrradianceConvolutionShader = gShaderPool.load(std::vector<ShaderFile>{
				{ ShaderType::Compute, "Resources/Shaders/IrradianceConvolutionCS.glsl" }
			});

			Texture2DParams params;
			params.wrapS = TextureWrap::ClampToEdge;
			params.wrapT = TextureWrap::ClampToEdge;
			params.sRGB = false;
			params.hasAlpha = false;
			params.genMips = false;
			IBL::gBRDFLUT = gTexturePool.decode("Resources/brdf_lut.ktx", params);

			IBL::gPrefilteredEnvMapShader = gShaderPool.load(std::vector<ShaderFile>{
				{ ShaderType::Compute, "Resources/Shaders/PrefilterEnvMapCS.glsl" }
			});

			IBL::gDefaultIrradiance = gCubemapPool.hdr_to_cubemap(
				create_1x1_texture(0, 0, 0, 255), 1);
			IBL::gDefaultPrefiltered = gCubemapPool.hdr_to_cubemap(
				create_1x1_texture(255, 0, 0, 255), 1);

			return
				/* --- Default Material --- */
				gMaterialPool.is_valid(gDefaultMaterial) &&
				gTexturePool.is_valid(gDefaultAlbedo) &&
				gTexturePool.is_valid(gDefaultNormal) &&
				gTexturePool.is_valid(gDefaultMR) &&
				gTexturePool.is_valid(gDefaultAO) &&
				gTexturePool.is_valid(gDefaultEmissive) &&
				/* --- Fullscreen Triangle --- */
				gMaterialPool.is_valid(gFullscreenMaterial) &&
				/* --- UBOs --- */
				gFrameUBO != 0 && gLightsUBO != 0 &&
				/* --- Skybox --- */
				gShaderPool.is_valid(gSkyboxShader) &&
				/* --- Forward+ --- */
				gShaderPool.is_valid(ForwardPlus::gDepthOnlyShader) &&
				gShaderPool.is_valid(ForwardPlus::gDepthDebugShader) &&
				gShaderPool.is_valid(ForwardPlus::gLightCullingShader) &&
				/* --- IBL --- */
				gShaderPool.is_valid(IBL::gEquirectToCubemapShader) &&
				gShaderPool.is_valid(IBL::gIrradianceConvolutionShader) &&
				gTexturePool.is_valid(IBL::gBRDFLUT) &&
				gShaderPool.is_valid(IBL::gPrefilteredEnvMapShader);
		}

		bool init_forward_plus()
		{
			using namespace ForwardPlus;

			auto winState = Window::get_state();
			gWidth = winState.width;
			gHeight = winState.height;

			gDepthPrePass.emplace();
			bool result = gDepthPrePass->init(gWidth, gHeight);

			gLightCullingPass.emplace();
			gLightCullingPass->init(gWidth, gHeight);

			return result;
		}
	} // anonymous namespace

	void render()
	{
		using namespace ForwardPlus;

		/* --- Scene --- */
		auto scene = ECS::get_active_scene();
		static bool sceneMissingLogged = false;
		if (!scene)
		{
			if (!sceneMissingLogged)
			{
				Logger::Internal::warn("No scene found to render from");
				sceneMissingLogged = true;
			}
			return;
		}
		sceneMissingLogged = false;

		/* --- Camera --- */
		auto camEOpt = scene->find_first<ECS::CameraComponent>();
		static bool camMissingLogged = false;
		if (!camEOpt)
		{
			if (!camMissingLogged)
			{
				Logger::Internal::warn("No camera component found in scene");
				camMissingLogged = true;
			}
			return;
		}
		camMissingLogged = false;

		auto camE = *camEOpt;
		Camera cam = camE.get<ECS::CameraComponent>().camera; // intended copy

		if (camE.has<ECS::TransformComponent>())
		{
			auto& camET = camE.get<ECS::TransformComponent>();
			cam.position += camET.position;
		}

		auto view = cam.get_view();
		auto proj = cam.get_proj();
		CameraData camData{
			.position = cam.position,
			.view = view, .proj = proj,
			.invProj = glm::inverse(proj),
			.viewProj = proj * view };

		/* --- Frame UBO --- */
		FrameGPUData frameData{
			.view = camData.view,
			.proj = camData.proj,
			.invProj = camData.invProj,
			.viewProj = camData.viewProj,
			.camPos = camData.position,
			.tileCountX = gLightCullingPass->get_tile_count_x(),
			.screenWidth = gWidth, .screenHeight = gHeight };
		glNamedBufferSubData(gFrameUBO, 0, sizeof(FrameGPUData), &frameData);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, gFrameUBO);

		/* --- IBL --- */
		if (auto iblEOpt = scene->find_first<ECS::IBLComponent>())
		{
			IBL::gIBLData = iblEOpt->get<ECS::IBLComponent>().iblData;
			IBL::gRenderSkybox = iblEOpt->get<ECS::IBLComponent>().skybox;
		}
		else
		{
			IBL::gIBLData = {};
			IBL::gRenderSkybox = false;
		}

		/* --- Lights --- */
		std::vector<DirLight> dirLights;
		std::vector<PointLight> pointLights;
		std::vector<SpotLight> spotLights;
		dirLights.reserve(MAX_DIR_LIGHTS);

		for (auto&& [id, dlC] : scene->view<ECS::DirLightComponent>())
		{
			if (dirLights.size() >= MAX_DIR_LIGHTS)
				break;

			dirLights.push_back(dlC.dirLight);
		}

		for (auto&& [id, plC] : scene->view<ECS::PointLightComponent>())
		{
			PointLight pl = plC.pointLight;

			auto e = scene->wrap(id);
			if (e.has<ECS::TransformComponent>())
				pl.position += e.get<ECS::TransformComponent>().position;

			pointLights.push_back(pl);
		}

		for (auto&& [id, slC] : scene->view<ECS::SpotLightComponent>())
		{
			SpotLight sl = slC.spotLight;

			auto e = scene->wrap(id);
			if (e.has<ECS::TransformComponent>())
			{
				auto& t = e.get<ECS::TransformComponent>();
				sl.position += t.position;
			}

			spotLights.push_back(sl);
		}

		/* --- Lights UBO --- */
		LightsGPUData lightsData{};
		lightsData.dirLightCount = static_cast<u32>(dirLights.size());

		for (u32 i = 0; i < lightsData.dirLightCount; ++i)
		{
			const auto& l = dirLights[i];
			lightsData.dirLights[i] = {
				glm::vec4(l.direction, l.intensity),
				glm::vec4(l.color, 0.0f),
			};
		}

		glNamedBufferSubData(gLightsUBO, 0, sizeof(LightsGPUData), &lightsData);
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, gLightsUBO);

		/* --- RenderObject --- */
		std::vector<RenderObject> renderObjects;
		std::vector<glm::mat4> modelMats;
		for (auto&& [id, tC, roC] :
			scene->view<ECS::TransformComponent,
			ECS::RenderObjectComponent>())
		{
			if (tC.dirty) ECS::update_world_transform(*scene, id);
			renderObjects.push_back(roC.renderObj);
			modelMats.push_back(tC.worldMatrix);
		}

		/* --- Depth Pre-Pass --- */
		glEnable(GL_DEPTH_TEST);
		gDepthPrePass->execute(camData, renderObjects, modelMats);
#if 0
		gDepthPrePass->debug_draw(gWidth, gHeight);
		return;
#endif
		gDepthPrePass->blit_depth_to(gWidth, gHeight, 0);

		/* --- Light Culling Pass --- */
		gLightCullingPass->execute(
			camData, pointLights, spotLights,
			gDepthPrePass->get_depth_id());

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5,
			gLightCullingPass->get_light_index_list_ssbo());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6,
			gLightCullingPass->get_light_grid_ssbo());

		/* --- Forward Pass --- */
		// TODO: These gl calls should live in draw()
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, gWidth, gHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LEQUAL);
		//glDepthMask(GL_FALSE); // TODO: Requires scene FBO

		for (size_t i = 0; i < renderObjects.size(); ++i)
		{
			draw({ .renderObj = renderObjects[i],
				   .camData = camData,
				   .modelMat = modelMats[i],
				   .normalMat = glm::transpose(glm::inverse(glm::mat3(modelMats[i])))
				});
		}

		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LEQUAL);

#if 0
		gLightCullingPass->debug_draw(gWidth, gHeight);
#endif

		if (IBL::gRenderSkybox) draw_skybox(cam);
	}

	bool init()
	{
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			Logger::Internal::critical("Failed to initialize GLAD");
			return false;
		}

		if (!load_engine_resources())
		{
			Logger::Internal::error("Failed to create engine resources");
			return false;
		}

		if (!init_forward_plus())
		{
			Logger::Internal::error("Failed to initialize Forward+ resources");
			return false;
		}

		return true;
	}

	void shutdown()
	{
		glDeleteVertexArrays(1, &gFullscreenVAO);
		glDeleteVertexArrays(1, &gSkyboxVAO);
		glDeleteBuffers(1, &gSkyboxVBO);

		glDeleteBuffers(1, &gFrameUBO);
		glDeleteBuffers(1, &gLightsUBO);

		Material::destroy_ssbo();

		gFullscreenVAO = 0;
		gSkyboxVAO = 0;
		gSkyboxVBO = 0;

		ForwardPlus::gDepthPrePass.reset();
		ForwardPlus::gLightCullingPass.reset();

		clear_all_resources();
	}

	void resize(u32 width, u32 height)
	{
		using namespace ForwardPlus;

		glViewport(0, 0,
			static_cast<i32>(width),
			static_cast<i32>(height));

		gDepthPrePass->resize(width, height);
		gLightCullingPass->resize(width, height);

		gWidth = width;
		gHeight = height;
	}

	void set_clear_color(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	namespace RES
	{
		MeshPool& get_mesh_pool() noexcept { return gMeshPool; }
		ShaderPool& get_shader_pool() noexcept { return gShaderPool; }
		TexturePool& get_texture_pool() noexcept { return gTexturePool; }
		CubemapPool& get_cubemap_pool()  noexcept { return gCubemapPool; }
		MaterialPool& get_material_pool() noexcept { return gMaterialPool; }
		ModelPool& get_model_pool() noexcept { return gModelPool; }

		ShaderHandle get_default_shader() noexcept { return gDefaultShader; }
		TextureHandle get_default_albedo() noexcept { return gDefaultAlbedo; }
		TextureHandle get_default_normal() noexcept { return gDefaultNormal; }
		TextureHandle get_default_mr() noexcept { return gDefaultMR; }
		TextureHandle get_default_ao() noexcept { return gDefaultAO; }
		TextureHandle get_default_emissive() noexcept { return gDefaultEmissive; }
		MaterialHandle get_default_material() noexcept { return gDefaultMaterial; }

		void clear_all_resources() noexcept
		{
			gMeshPool.clear();
			gShaderPool.clear();
			gTexturePool.clear();
			gCubemapPool.clear();
			gMaterialPool.clear();
			gModelPool.clear();
		}

		void poll_hot_reloads() noexcept
		{
			gShaderPool.poll_hot_reload();
			gTexturePool.poll_hot_reload();
		}
	}
}