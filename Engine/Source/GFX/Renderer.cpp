#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Renderer.hpp>
#include <WinkEngine/GFX/Renderer/Data.hpp>
#include <WinkEngine/GFX/Renderer/Pipeline.hpp>

#include <WinkEngine/ECS/Scene.hpp>
#include <WinkEngine/ECS/Components/TransformComponent.hpp>
#include <WinkEngine/ECS/Components/RenderObjectComponent.hpp>
#include <WinkEngine/ECS/Components/CameraComponent.hpp>
#include <WinkEngine/ECS/Components/LightComponent.hpp>
#include <WinkEngine/ECS/Components/IBLComponent.hpp>
#include <WinkEngine/ECS/Systems/TransformSystem.hpp>

#include <GLFW/glfw3.h>

namespace Wink::GFX
{
	using namespace RES;

	Configuration gConfig;

	MeshPool gMeshPool;
	ShaderPool gShaderPool;
	TexturePool gTexturePool;
	CubemapPool gCubemapPool;
	MaterialPool gMaterialPool;
	ModelPool gModelPool;

	ShaderHandle gDefaultShader;
	TextureHandle gDefaultAlbedo;
	TextureHandle gDefaultNormal;
	TextureHandle gDefaultMR;
	TextureHandle gDefaultAO;
	TextureHandle gDefaultEmissive;
	MaterialHandle gDefaultMaterial;

	ShaderHandle gFullscreenShader;
	MaterialHandle gFullscreenMaterial;
	u32 gFullscreenVAO = 0;

	ShaderHandle gSkyboxShader;
	u32 gSkyboxVAO = 0;
	u32 gSkyboxVBO = 0;
	
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

			gFullscreenShader = gShaderPool.load(std::vector<ShaderFile>{
				{ ShaderType::Vertex, "Resources/Shaders/FullscreenVS.glsl" },
				{ ShaderType::Fragment, "Resources/Shaders/FullscreenFS.glsl" },
			});
			gFullscreenMaterial = gMaterialPool.create(gFullscreenShader);
			glCreateVertexArrays(1, &gFullscreenVAO);

			gSkyboxShader = gShaderPool.load(std::vector<ShaderFile>{
				{ ShaderType::Vertex, "Resources/Shaders/SkyboxVS.glsl" },
				{ ShaderType::Fragment, "../../../../Engine/Source/GFX/Shaders/SkyboxFS.glsl" },
			});
			create_skybox_geometry();

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

			return gMaterialPool.is_valid(gDefaultMaterial) &&
				gTexturePool.is_valid(gDefaultAlbedo) &&
				gTexturePool.is_valid(gDefaultNormal) &&
				gTexturePool.is_valid(gDefaultMR) &&
				gTexturePool.is_valid(gDefaultAO) &&
				gTexturePool.is_valid(gDefaultEmissive) &&
				gMaterialPool.is_valid(gFullscreenMaterial) &&
				gShaderPool.is_valid(gSkyboxShader) &&
				gShaderPool.is_valid(IBL::gEquirectToCubemapShader) &&
				gShaderPool.is_valid(IBL::gIrradianceConvolutionShader) &&
				gTexturePool.is_valid(IBL::gBRDFLUT) &&
				gShaderPool.is_valid(IBL::gPrefilteredEnvMapShader);
		}
	} // anonymous namespace

	void render()
	{
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

		CameraData camData{ .position = cam.position,
			.viewProj = cam.get_proj() * cam.get_view() };

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
		pointLights.reserve(MAX_POINT_LIGHTS);
		spotLights.reserve(MAX_SPOT_LIGHTS);

		for (auto&& [id, dlC] : scene->view<ECS::DirLightComponent>())
		{
			if (dirLights.size() >= MAX_DIR_LIGHTS)
				break;

			dirLights.push_back(dlC.dirLight);
		}

		for (auto&& [id, plC] : scene->view<ECS::PointLightComponent>())
		{
			if (pointLights.size() >= MAX_POINT_LIGHTS)
				break;

			PointLight pl = plC.pointLight;

			auto e = scene->wrap(id);
			if (e.has<ECS::TransformComponent>())
				pl.position += e.get<ECS::TransformComponent>().position;

			pointLights.push_back(pl);
		}

		for (auto&& [id, slC] : scene->view<ECS::SpotLightComponent>())
		{
			if (spotLights.size() >= MAX_SPOT_LIGHTS)
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
			scene->view<ECS::TransformComponent, ECS::RenderObjectComponent>())
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

		return true;
	}

	void shutdown()
	{
		glDeleteVertexArrays(1, &gFullscreenVAO);
		glDeleteVertexArrays(1, &gSkyboxVAO);
		glDeleteBuffers(1, &gSkyboxVBO);

		gFullscreenVAO = 0;
		gSkyboxVAO = 0;
		gSkyboxVBO = 0;

		clear_all_resources();
	}

	void resize(u32 width, u32 height)
	{
		glViewport(0, 0,
			static_cast<i32>(width),
			static_cast<i32>(height));
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