#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Renderer.hpp>

#include <WinkEngine/ECS/Scene.hpp>
#include <WinkEngine/ECS/Components/TransformComponent.hpp>
#include <WinkEngine/ECS/Components/RenderObjectComponent.hpp>
#include <WinkEngine/ECS/Components/CameraComponent.hpp>
#include <WinkEngine/ECS/Components/LightComponent.hpp>
#include <WinkEngine/ECS/Components/IBLComponent.hpp>
#include <WinkEngine/ECS/Systems/TransformSystem.hpp>

#include <WinkEngine/Core/Logger.hpp>
#include <WinkEngine/Core/Profiler.hpp>
#include <GLFW/glfw3.h>

namespace Wink::GFX
{
	namespace IBL
	{
		using namespace Resource;

		ShaderHandle gEquirectToCubemapShader;
		ShaderHandle gIrradianceConvolutionShader;
		ShaderHandle gPrefilteredEnvMapShader;
		TextureHandle gBRDFLUT;

		IBLData gIBLData;
		bool gRenderSkybox;
	}

	namespace
	{
		using namespace Resource;

		Configuration gConfig;

		MeshPool gMeshPool;
		ShaderPool gShaderPool;
		TexturePool gTexturePool;
		CubemapPool gCubemapPool;
		MaterialPool gMaterialPool;
		ModelPool gModelPool;

		ShaderHandle gDefaultShader;
		MaterialHandle gDefaultMaterial;

		ShaderHandle gFullscreenShader;
		MaterialHandle gFullscreenMaterial;
		GLuint gFullscreenVAO = 0;

		ShaderHandle gSkyboxShader;
		GLuint gSkyboxVAO = 0;

		constexpr float SKYBOX_VERTICES[] =
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
			u32 vbo;
			glCreateVertexArrays(1, &gSkyboxVAO);
			glCreateBuffers(1, &vbo);

			glNamedBufferStorage(vbo,
				sizeof(SKYBOX_VERTICES), SKYBOX_VERTICES, 0);

			glVertexArrayVertexBuffer(gSkyboxVAO, 0, vbo, 0, 3 * sizeof(float));
			glEnableVertexArrayAttrib(gSkyboxVAO, 0);
			glVertexArrayAttribFormat(gSkyboxVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
			glVertexArrayAttribBinding(gSkyboxVAO, 0, 0);
		}

		bool load_engine_resources()
		{
			gDefaultShader = gShaderPool.load(std::vector<ShaderFile>{
				{ ShaderType::Vertex, "Resources/Shaders/DefaultVS.glsl" },
				{ ShaderType::Fragment, "Resources/Shaders/DefaultFS.glsl" },
			});
			gDefaultMaterial = gMaterialPool.create(gDefaultShader);

			gFullscreenShader = gShaderPool.load(std::vector<ShaderFile>{
				{ ShaderType::Vertex, "Resources/Shaders/FullscreenVS.glsl" },
				{ ShaderType::Fragment, "Resources/Shaders/FullscreenFS.glsl" },
			});
			gFullscreenMaterial = gMaterialPool.create(gFullscreenShader);
			glCreateVertexArrays(1, &gFullscreenVAO);

			gSkyboxShader = gShaderPool.load(std::vector<ShaderFile>{
				{ ShaderType::Vertex, "Resources/Shaders/SkyboxVS.glsl" },
				{ ShaderType::Fragment, "Resources/Shaders/SkyboxFS.glsl" },
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

			return gMaterialPool.is_valid(gDefaultMaterial) &&
				gMaterialPool.is_valid(gFullscreenMaterial) &&
				gShaderPool.is_valid(gSkyboxShader) &&
				gShaderPool.is_valid(IBL::gEquirectToCubemapShader) &&
				gShaderPool.is_valid(IBL::gIrradianceConvolutionShader) &&
				gTexturePool.is_valid(IBL::gBRDFLUT) &&
				gShaderPool.is_valid(IBL::gPrefilteredEnvMapShader);
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

			glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		}

		void draw_fullscreen(TextureHandle tex)
		{
			auto& matPool = get_material_pool();
			auto& texPool = get_texture_pool();

			matPool.apply(gFullscreenMaterial);
			texPool.bind(tex, 0);

			glBindVertexArray(gFullscreenVAO);
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}

		void draw_skybox(const Camera& cam)
		{
			const TextureCubemap* envMap =
				gCubemapPool.try_get(IBL::gIBLData.envMap);
			if (!envMap || !envMap->is_valid()) return;

			const ShaderProgram* shader = gShaderPool.try_get(gSkyboxShader);
			if (!shader || !shader->is_valid()) return;

			const glm::mat4 rotView = glm::mat4(glm::mat3(cam.get_view()));
			const glm::mat4 viewProj = cam.get_proj() * rotView;

			glDepthFunc(GL_LEQUAL);
			glDepthMask(GL_FALSE);

			shader->use();
			shader->set("uViewProj", viewProj);
			shader->set("uSkybox", 0);
			envMap->bind(0);

			glBindVertexArray(gSkyboxVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glBindVertexArray(0);

			glDepthMask(gConfig.depthWrite ? GL_TRUE : GL_FALSE);
			glDepthFunc(gConfig.depthFunc);
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

			/* --- IBL --- */
			const TextureCubemap* irradiance = gCubemapPool.try_get(IBL::gIBLData.irradianceMap);
			const TextureCubemap* prefiltered = gCubemapPool.try_get(IBL::gIBLData.prefilteredEnvMap);
			const Texture2D* brdfLUT = gTexturePool.try_get(IBL::gBRDFLUT);

			const bool hasIBL = irradiance && irradiance->is_valid()
				&& prefiltered && prefiltered->is_valid()
				&& brdfLUT && brdfLUT->is_valid();

			shader->set("uHasIBL", hasIBL);
			if (hasIBL)
			{
				irradiance->bind(12);
				prefiltered->bind(13);
				brdfLUT->bind(14);
				shader->set("uIrradianceMap", 12);
				shader->set("uPrefilteredMap", 13);
				shader->set("uBRDFLUT", 14);
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

		if (IBL::gRenderSkybox) draw_skybox(cam);
	}

	void render_fullscreen_texture(Resource::TextureHandle tex)
	{
		draw_fullscreen(tex);
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
			Logger::Internal::error("Failed to create engine materials");
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
		CubemapPool& get_cubemap_pool()  noexcept { return gCubemapPool; }
		MaterialPool& get_material_pool() noexcept { return gMaterialPool; }
		ModelPool& get_model_pool() noexcept { return gModelPool; }

		ShaderHandle get_default_shader() noexcept { return gDefaultShader; }
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

	namespace IBL
	{
		namespace Internal
		{
			Resource::CubemapHandle equirect_to_cubemap(
				Resource::TextureHandle hdr, u32 faceSize)
			{
				using namespace Resource;
				ENGINE_ZONE_NAME("Equirect To Cubemap");

				const Texture2D* tex = gTexturePool.try_get(hdr);
				if (!tex || !tex->is_valid())
				{
					Logger::Internal::error(
						"Invalid HDR Texture");
					return CubemapHandle();
				}

				CubemapSpec spec;
				spec.width = faceSize;
				spec.height = faceSize;
				spec.internalFormat = GL_RGBA16F;
				spec.minFilter = TextureFilter::LinearMipmapLinear;
				spec.magFilter = TextureFilter::Linear;
				spec.wrapMode = TextureWrap::ClampToEdge;
				spec.generateMipmaps = true;

				CubemapHandle outHandle = gCubemapPool.allocate(spec);
				TextureCubemap* cubemap = gCubemapPool.try_get(outHandle);
				if (!cubemap || !cubemap->is_valid())
				{
					Logger::Internal::error(
						"Failed to allocate output cubemap");
					return CubemapHandle();
				}

				const ShaderProgram* cs = gShaderPool.try_get(gEquirectToCubemapShader);
				if (!cs || !cs->is_valid()) return CubemapHandle();

				cs->use();

				tex->bind(0);
				cs->set("uEquirect", 0);

				cubemap->bind_image(1, 0, GL_TRUE, GL_WRITE_ONLY, GL_RGBA16F);

				const u32 groups = (faceSize + 7) / 8;
				cs->dispatch(groups, groups, 6);
				cs->memory_barrier(GL_TEXTURE_FETCH_BARRIER_BIT |
					GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

				glBindImageTexture(1, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

				cubemap->generate_mipmaps();

				return outHandle;
			}
		} // namespace Internal

		Resource::TextureHandle get_brdf_lut()
		{
			return gBRDFLUT;
		}

		Resource::CubemapHandle bake_irradiance_map(
			Resource::CubemapHandle envCubemap, u32 faceSize)
		{
			using namespace Resource;
			ENGINE_ZONE_NAME("Bake Irradiance");

			const TextureCubemap* env = gCubemapPool.try_get(envCubemap);
			if (!env || !env->is_valid())
			{
				Logger::Internal::error("Invalid environment cubemap handle");
				return CubemapHandle();
			}

			const ShaderProgram* cs = gShaderPool.try_get(gIrradianceConvolutionShader);
			if (!cs || !cs->is_valid()) return CubemapHandle();

			CubemapSpec spec;
			spec.width = faceSize;
			spec.height = faceSize;
			spec.internalFormat = GL_RGBA16F;
			spec.minFilter = TextureFilter::Linear;
			spec.magFilter = TextureFilter::Linear;
			spec.wrapMode = TextureWrap::ClampToEdge;
			spec.generateMipmaps = false;
			spec.mipLevels = 1;

			CubemapHandle outHandle = gCubemapPool.allocate(spec);
			TextureCubemap* irradiance = gCubemapPool.try_get(outHandle);
			if (!irradiance || !irradiance->is_valid())
			{
				Logger::Internal::error("Failed to allocate irradiance cubemap");
				return CubemapHandle();
			}

			cs->use();

			env->bind(0);
			cs->set("uEnvironment", 0);

			irradiance->bind_image(1, 0, GL_TRUE, GL_WRITE_ONLY, GL_RGBA16F);

			const u32 groups = (faceSize + 7) / 8;

			cs->dispatch(groups, groups, 6);
			cs->memory_barrier(GL_TEXTURE_FETCH_BARRIER_BIT |
				GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			glBindImageTexture(1, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
			return outHandle;
		}

		Resource::CubemapHandle bake_prefiltered_env_map(
			Resource::CubemapHandle envCubemap, u32 faceSize,
			u32 mipLevels, u32 sampleCount)
		{
			using namespace Resource;
			ENGINE_ZONE_NAME("Bake Prefiltered Env Map");

			const TextureCubemap* env = gCubemapPool.try_get(envCubemap);
			if (!env || !env->is_valid())
			{
				Logger::Internal::error("Invalid environment cubemap handle");
				return CubemapHandle();
			}

			const ShaderProgram* cs = gShaderPool.try_get(gPrefilteredEnvMapShader);
			if (!cs || !cs->is_valid()) return CubemapHandle();

			CubemapSpec spec;
			spec.width = faceSize;
			spec.height = faceSize;
			spec.internalFormat = GL_RGBA16F;
			spec.minFilter = TextureFilter::LinearMipmapLinear;
			spec.magFilter = TextureFilter::Linear;
			spec.wrapMode = TextureWrap::ClampToEdge;
			spec.generateMipmaps = false;
			spec.mipLevels = mipLevels;

			CubemapHandle outHandle = gCubemapPool.allocate(spec);
			TextureCubemap* prefiltered = gCubemapPool.try_get(outHandle);
			if (!prefiltered || !prefiltered->is_valid())
			{
				Logger::Internal::error("Failed to allocate prefiltered env cubemap");
				return CubemapHandle();
			}

			cs->use();
			env->bind(0);
			cs->set("uEnvironment", 0);
			cs->set("uSampleCount", static_cast<u32>(sampleCount));

			for (u32 mip = 0; mip < mipLevels; ++mip)
			{
				const u32 mipSize = faceSize >> mip;
				const float roughness = static_cast<float>(mip) / static_cast<float>(mipLevels - 1);

				cs->set("uRoughness", roughness);
				cs->set("uFaceSize", static_cast<i32>(mipSize));

				glBindImageTexture(1, prefiltered->get_id(),
					static_cast<GLint>(mip), GL_TRUE, 0,
					GL_WRITE_ONLY, GL_RGBA16F);

				const u32 groups = std::max(1u, (mipSize + 7) / 8);
				cs->dispatch(groups, groups, 6);
				cs->memory_barrier(GL_TEXTURE_FETCH_BARRIER_BIT |
					GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			}

			glBindImageTexture(1, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
			return outHandle;
		}
	}
}