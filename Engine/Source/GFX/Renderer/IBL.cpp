#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Renderer.hpp>
#include <WinkEngine/GFX/Renderer/IBL.hpp>

#include <WinkEngine/Core/Logger.hpp>
#include <WinkEngine/Core/Profiler.hpp>

namespace Wink::GFX
{
	using namespace RES;

	extern ShaderPool gShaderPool;
	extern TexturePool gTexturePool;
	extern CubemapPool gCubemapPool;

	namespace IBL
	{
		extern ShaderHandle gEquirectToCubemapShader;
		extern ShaderHandle gIrradianceConvolutionShader;
		extern ShaderHandle gPrefilteredEnvMapShader;

		CubemapHandle equirect_to_cubemap(
			TextureHandle hdr, u32 faceSize)
		{
			ENGINE_ZONE_NAME("Equirect To Cubemap");

			const Texture2D* tex = gTexturePool.try_get(hdr);
			if (!tex || !tex->is_valid())
			{
				Logger::Internal::error("Invalid HDR texture");
				return {};
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
				Logger::Internal::error("Failed to allocate output cubemap");
				return {};
			}

			const ShaderProgram* cs = gShaderPool.try_get(gEquirectToCubemapShader);
			if (!cs || !cs->is_valid()) return {};

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

		CubemapHandle bake_irradiance_map(
			CubemapHandle envCubemap, u32 faceSize)
		{
			ENGINE_ZONE_NAME("Bake Irradiance");

			const TextureCubemap* env = gCubemapPool.try_get(envCubemap);
			if (!env || !env->is_valid())
			{
				Logger::Internal::error("Invalid environment cubemap handle");
				return {};
			}

			const ShaderProgram* cs = gShaderPool.try_get(gIrradianceConvolutionShader);
			if (!cs || !cs->is_valid()) return {};

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
				return {};
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

		CubemapHandle bake_prefiltered_env_map(
			CubemapHandle envCubemap, u32 faceSize,
			u32 mipLevels, u32 sampleCount)
		{
			ENGINE_ZONE_NAME("Bake Prefiltered Env Map");

			assert(mipLevels > 1);

			const TextureCubemap* env = gCubemapPool.try_get(envCubemap);
			if (!env || !env->is_valid())
			{
				Logger::Internal::error("Invalid environment cubemap handle");
				return {};
			}

			const ShaderProgram* cs = gShaderPool.try_get(gPrefilteredEnvMapShader);
			if (!cs || !cs->is_valid()) return {};

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
				return {};
			}

			cs->use();
			env->bind(0);
			cs->set("uEnvironment", 0);
			cs->set("uSampleCount", sampleCount);

			for (u32 mip = 0; mip < mipLevels; ++mip)
			{
				const u32 mipSize = faceSize >> mip;
				const float roughness = static_cast<float>(mip) / static_cast<float>(mipLevels - 1);

				cs->set("uRoughness", roughness);
				cs->set("uFaceSize", static_cast<i32>(mipSize));

				glBindImageTexture(1, prefiltered->get_id(),
					static_cast<i32>(mip), GL_TRUE, 0,
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