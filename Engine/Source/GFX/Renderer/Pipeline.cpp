#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Renderer/Pipeline.hpp>
#include <WinkEngine/GFX/Renderer/Pipeline/ShadowPass.hpp>
#include <WinkEngine/GFX/Renderer/IBL.hpp>
#include <WinkEngine/GFX/RES/MeshPool.hpp>
#include <WinkEngine/GFX/RES/ShaderPool.hpp>
#include <WinkEngine/GFX/RES/TexturePool.hpp>
#include <WinkEngine/GFX/RES/CubemapPool.hpp>
#include <WinkEngine/GFX/RES/MaterialPool.hpp>
#include <WinkEngine/Core/Logger.hpp>

namespace Wink::GFX
{
	using namespace RES;

	extern MeshPool gMeshPool;
	extern ShaderPool gShaderPool;
	extern TexturePool gTexturePool;
	extern CubemapPool gCubemapPool;
	extern MaterialPool gMaterialPool;

	extern MaterialHandle gDefaultMaterial;

	extern ShaderHandle gSkyboxShader;
	extern u32 gSkyboxVAO;

	namespace IBL
	{
		extern TextureHandle gBRDFLUT;

		extern CubemapHandle gDefaultIrradiance;
		extern CubemapHandle gDefaultPrefiltered;

		extern IBLData gIBLData;
	}

	namespace
	{
		struct DirLightNames { std::string direction, intensity, color; };
		struct PointLightNames { std::string position, intensity, color, radius; };
		struct SpotLightNames
		{
			std::string position, range, direction,
				innerCutoff, color, outerCutoff, intensity;
		};

		template<typename NamesT, typename BuildFn>
		auto make_light_names(u32 count, std::string_view arrayName, BuildFn&& build)
		{
			std::vector<NamesT> names;
			names.reserve(count);

			for (u32 i = 0; i < count; ++i)
			{
				std::string base(arrayName);
				base += '[';
				base += std::to_string(i);
				base += "].";
				names.push_back(build(base));
			}

			return names;
		}

		const std::vector<DirLightNames> DIR_LIGHT_NAMES = make_light_names<DirLightNames>(
			MAX_DIR_LIGHTS, "uDirLights", [](const std::string& b)
			{
				return DirLightNames{ b + "direction", b + "intensity", b + "color" };
			});
	} // anonymous namespace

	void draw_skybox(const Camera& cam)
	{
		const TextureCubemap* envMap =
			gCubemapPool.try_get(IBL::gIBLData.envMap);
		if (!envMap || !envMap->is_valid()) return;

		const ShaderProgram* shader = gShaderPool.try_get(gSkyboxShader);
		if (!shader || !shader->is_valid()) return;

		const glm::mat4 rotView = glm::mat4(glm::mat3(cam.get_view()));
		const glm::mat4 viewProj = cam.get_proj() * rotView;

		shader->use();
		shader->set("uViewProj", viewProj);
		shader->set("uSkybox", 0);
		envMap->bind(0);

		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);

		glBindVertexArray(gSkyboxVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);

		glEnable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);
	}

	void draw(const DrawData& drawData)
	{
		auto material = gMaterialPool.is_valid(drawData.renderObj.material) ?
			drawData.renderObj.material : gDefaultMaterial;
		auto mesh = drawData.renderObj.mesh;

		const Material* mat = gMaterialPool.try_get(material);
		assert(mat != nullptr);

		auto* shader = gShaderPool.try_get(mat->shader);
		if (!shader || !shader->is_valid())
		{
			Logger::Internal::error("Material has no valid shader; skipping draw");
			return;
		}

		gMaterialPool.apply(material);

		/* --- Per-Object Uniforms --- */
		shader->set("uModel", drawData.modelMat);
		shader->set("uNormalMatrix", drawData.normalMat);

		/* --- IBL --- */
		TextureCubemap* irradiance = gCubemapPool.try_get(IBL::gIBLData.irradianceMap);
		TextureCubemap* prefiltered = gCubemapPool.try_get(IBL::gIBLData.prefilteredEnvMap);
		const Texture2D* brdfLUT = gTexturePool.try_get(IBL::gBRDFLUT);

		const bool hasIBL = irradiance && irradiance->is_valid()
			&& prefiltered && prefiltered->is_valid();

		shader->set("uHasIBL", hasIBL);
		if (!hasIBL)
		{
			irradiance = gCubemapPool.try_get(IBL::gDefaultIrradiance);
			prefiltered = gCubemapPool.try_get(IBL::gDefaultPrefiltered);
		}

		assert(irradiance && prefiltered && brdfLUT);

		irradiance->bind(12);
		prefiltered->bind(13);
		brdfLUT->bind(14);
		shader->set("uIrradianceMap", 12);
		shader->set("uPrefilteredMap", 13);
		shader->set("uBRDFLUT", 14);

		/* --- Draw --- */
		glBindVertexArray(gMeshPool.get_vao_id(mesh));
		glDrawElements(GL_TRIANGLES,
			static_cast<i32>(gMeshPool.get_index_count(mesh)),
			GL_UNSIGNED_INT, nullptr);
	}
}