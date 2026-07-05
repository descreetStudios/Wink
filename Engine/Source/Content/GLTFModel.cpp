#include <WinkEngine/pch.hpp>
#include <WinkEngine/Content/Model.hpp>
#include <WinkEngine/GFX/Renderer.hpp>
#include <WinkEngine/Core/Logger.hpp>
#include <WinkEngine/Core/Profiler.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>

#include <mikktspace.h>

namespace Wink::Content
{
	using namespace GFX;
	using namespace RES;

	namespace
	{
		struct LoadProgress
		{
			std::vector<TextureHandle> textures;
			std::vector<MaterialHandle> materials;
			std::vector<MeshHandle> meshes;

			void rollback() const
			{
				for (TextureHandle h : textures) get_texture_pool().unload(h);
				for (MaterialHandle h : materials) get_material_pool().destroy(h);
				for (MeshHandle h : meshes) get_mesh_pool().unload(h);
			}
		};

		void generate_tangents_mikktspace(MeshData& data)
		{
			ENGINE_ZONE_NAME("MikkTSpace Generation");

			struct UserData { MeshData* mesh; };
			UserData ud{ &data };

			SMikkTSpaceInterface iface{};

			iface.m_getNumFaces = [](const SMikkTSpaceContext* ctx) -> i32
				{
					auto* ud = static_cast<UserData*>(ctx->m_pUserData);
					return static_cast<i32>(ud->mesh->indices.size() / 3);
				};

			iface.m_getNumVerticesOfFace = [](const SMikkTSpaceContext*, i32) -> i32
				{
					return 3;
				};

			iface.m_getPosition = [](const SMikkTSpaceContext* ctx,
				float out[], i32 face, i32 vert)
				{
					auto* ud = static_cast<UserData*>(ctx->m_pUserData);
					u32 idx = ud->mesh->indices[static_cast<size_t>(face) * 3 + vert];
					const glm::vec3& p = ud->mesh->vertices[idx].position;
					out[0] = p.x; out[1] = p.y; out[2] = p.z;
				};

			iface.m_getNormal = [](const SMikkTSpaceContext* ctx,
				float out[], i32 face, i32 vert)
				{
					auto* ud = static_cast<UserData*>(ctx->m_pUserData);
					u32 idx = ud->mesh->indices[static_cast<size_t>(face) * 3 + vert];
					const glm::vec3& n = ud->mesh->vertices[idx].normal;
					out[0] = n.x; out[1] = n.y; out[2] = n.z;
				};

			iface.m_getTexCoord = [](const SMikkTSpaceContext* ctx,
				float out[], i32 face, i32 vert)
				{
					auto* ud = static_cast<UserData*>(ctx->m_pUserData);
					u32 idx = ud->mesh->indices[static_cast<size_t>(face) * 3 + vert];
					const glm::vec2& uv = ud->mesh->vertices[idx].uv;
					out[0] = uv.x; out[1] = uv.y;
				};

			iface.m_setTSpaceBasic = [](const SMikkTSpaceContext* ctx,
				const float tangent[], float sign, i32 face, i32 vert)
				{
					auto* ud = static_cast<UserData*>(ctx->m_pUserData);
					u32 idx = ud->mesh->indices[static_cast<size_t>(face) * 3 + vert];
					ud->mesh->vertices[idx].tangent = {
						tangent[0], tangent[1], tangent[2], sign };
				};

			SMikkTSpaceContext ctx{ &iface, &ud };
			genTangSpaceDefault(&ctx);
		}

		std::span<const u8> resolve_embedded_bytes(
			const fastgltf::Asset& asset,
			const fastgltf::Image& image)
		{
			if (const auto* arr = std::get_if<
				fastgltf::sources::Array>(&image.data))
			{
				return { reinterpret_cast<const u8*>(
					arr->bytes.data()), arr->bytes.size() };
			}

			if (const auto* bv = std::get_if<
				fastgltf::sources::BufferView>(&image.data))
			{
				const fastgltf::BufferView& bufferView =
					asset.bufferViews[bv->bufferViewIndex];
				const fastgltf::Buffer& buffer =
					asset.buffers[bufferView.bufferIndex];

				if (const auto* bufArr = std::get_if<
					fastgltf::sources::Array>(&buffer.data))
				{
					const u8* base = reinterpret_cast<const u8*>(
						bufArr->bytes.data()) + bufferView.byteOffset;
					return { base, bufferView.byteLength };
				}
			}

			return {};
		}

		template <typename TexInfoT>
		std::optional<TextureHandle> load_material_texture(
			const fastgltf::Asset& asset,
			const fastgltf::Optional<TexInfoT>& texInfo,
			const fs::path& baseDir, const Texture2DParams& params,
			LoadProgress& progress)
		{
			ENGINE_ZONE_NAME("Load Material Texture");

			if (!texInfo.has_value()) return std::nullopt;

			const fastgltf::Texture& tex = asset.textures[texInfo->textureIndex];
			if (!tex.imageIndex.has_value()) return std::nullopt;

			const fastgltf::Image& image = asset.images[*tex.imageIndex];
			auto& texturePool = get_texture_pool();

			TextureHandle handle;

			if (const auto* uri = std::get_if<
				fastgltf::sources::URI>(&image.data))
			{
				const fs::path imagePath = baseDir / uri->uri.fspath();
				handle = texturePool.decode(imagePath, params);
			}
			else if (auto bytes = resolve_embedded_bytes(
				asset, image); !bytes.empty())
			{
				handle = texturePool.decode_from_memory(
					bytes.data(), bytes.size(), params);
			}
			else
			{
				Logger::Internal::error(
					"Unsupported/unresolvable glTF "
					"image source for image '{}'", image.name);
				return std::nullopt;
			}

			if (!handle.is_valid()) return std::nullopt;

			progress.textures.push_back(handle);
			return handle;
		}

		template <typename TexInfoT>
		u32 resolve_tex_coord(const TexInfoT& texInfo)
		{
			if (texInfo.transform && texInfo.transform->texCoordIndex.has_value())
				return static_cast<u32>(*texInfo.transform->texCoordIndex);
			return static_cast<u32>(texInfo.texCoordIndex);
		}

		[[nodiscard]] MaterialHandle build_material(
			const fastgltf::Asset& asset,
			size_t matIndex, const fs::path& baseDir,
			ShaderHandle shader, LoadProgress& progress)
		{
			ENGINE_ZONE_NAME("Material building");

			const fastgltf::Material& src = asset.materials[matIndex];
			auto& materialPool = get_material_pool();

			MaterialHandle handle = materialPool.create(shader);
			if (!materialPool.is_valid(handle))
			{
				Logger::Internal::error(
					"Failed to build material at index: '{}'", matIndex);
				return handle;
			}
			progress.materials.push_back(handle);

			auto* mat = materialPool.try_get(handle);

			/* --- Params --- */
			mat->params.baseColor = {
				src.pbrData.baseColorFactor[0],
				src.pbrData.baseColorFactor[1],
				src.pbrData.baseColorFactor[2],
				src.pbrData.baseColorFactor[3] };
			mat->params.metallic = src.pbrData.metallicFactor;
			mat->params.roughness = src.pbrData.roughnessFactor;
			mat->params.emissiveFactor = {
				src.emissiveFactor[0],
				src.emissiveFactor[1],
				src.emissiveFactor[2], 0.0f };

			/* --- Maps --- */
			const Texture2DParams albedoParams{ .sRGB = true };
			if (auto albedo = load_material_texture<fastgltf::TextureInfo>(
				asset, src.pbrData.baseColorTexture, baseDir, albedoParams, progress))
			{
				mat->textures.albedo = *albedo;
				mat->params.albedoTexCoord = resolve_tex_coord(*src.pbrData.baseColorTexture);
			}

			const Texture2DParams normalParams{ .sRGB = false, .hasAlpha = false };
			if (auto normal = load_material_texture<fastgltf::NormalTextureInfo>(
				asset, src.normalTexture, baseDir, normalParams, progress))
			{
				mat->textures.normal = *normal;
				mat->params.normalTexCoord = resolve_tex_coord(*src.normalTexture);
			}

			const Texture2DParams mrParams{ .sRGB = false };
			if (auto mr = load_material_texture<fastgltf::TextureInfo>(
				asset, src.pbrData.metallicRoughnessTexture, baseDir, mrParams, progress))
			{
				mat->textures.mr = *mr;
				mat->params.mrTexCoord = resolve_tex_coord(*src.pbrData.metallicRoughnessTexture);
			}

			const Texture2DParams aoParams{ .sRGB = false };
			if (auto ao = load_material_texture<fastgltf::OcclusionTextureInfo>(
				asset, src.occlusionTexture, baseDir, aoParams, progress))
			{
				mat->textures.ao = *ao;
				mat->params.aoTexCoord = resolve_tex_coord(*src.occlusionTexture);
			}

			const Texture2DParams emissiveParams{ .sRGB = true };
			if (auto em = load_material_texture<fastgltf::TextureInfo>(
				asset, src.emissiveTexture, baseDir, emissiveParams, progress))
			{
				mat->textures.emissive = *em;
				mat->params.emissiveTexCoord = resolve_tex_coord(*src.emissiveTexture);
			}

			return handle;
		}

		[[nodiscard]] std::optional<MeshHandle> build_mesh(
			const fastgltf::Asset& asset,
			const fastgltf::Primitive& prim,
			LoadProgress& progress)
		{
			ENGINE_ZONE_NAME("Build Mesh");

			if (prim.type != fastgltf::PrimitiveType::Triangles)
			{
				Logger::Internal::error(
					"glTF primitive is not a triangle list. Skipping");
				return std::nullopt;
			}

			const auto* posIt = prim.findAttribute("POSITION");
			if (posIt == prim.attributes.end()) return std::nullopt;

			const fastgltf::Accessor& posAccessor =
				asset.accessors[posIt->accessorIndex];

			MeshData data;
			data.vertices.resize(posAccessor.count);

			fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, posAccessor,
				[&](glm::vec3 v, size_t i) { data.vertices[i].position = v; });

			if (const auto* normIt = prim.findAttribute("NORMAL");
				normIt != prim.attributes.end())
			{
				fastgltf::iterateAccessorWithIndex<glm::vec3>(
					asset, asset.accessors[normIt->accessorIndex],
					[&](glm::vec3 v, size_t i) { data.vertices[i].normal = v; });
			}

			if (const auto* uvIt = prim.findAttribute("TEXCOORD_0");
				uvIt != prim.attributes.end())
			{
				fastgltf::iterateAccessorWithIndex<glm::vec2>(
					asset, asset.accessors[uvIt->accessorIndex],
					[&](glm::vec2 v, size_t i) { data.vertices[i].uv = { v.x, 1.0f - v.y }; });
			}

			if (const auto* uv1It = prim.findAttribute("TEXCOORD_1");
				uv1It != prim.attributes.end())
			{
				fastgltf::iterateAccessorWithIndex<glm::vec2>(
					asset, asset.accessors[uv1It->accessorIndex],
					[&](glm::vec2 v, size_t i) { data.vertices[i].uv1 = { v.x, 1.0f - v.y }; });
			}

			if (!prim.indicesAccessor.has_value())
			{
				Logger::Internal::error(
					"glTF primitive has no index accessor. "
					"Skipping (non-indexed not supported)");
				return std::nullopt;
			}

			const fastgltf::Accessor& idxAccessor =
				asset.accessors[*prim.indicesAccessor];
			data.indices.resize(idxAccessor.count);

			fastgltf::iterateAccessorWithIndex<u32>(asset, idxAccessor,
				[&](u32 v, size_t i) { data.indices[i] = v; });

			if (const auto* tanIt = prim.findAttribute("TANGENT");
				tanIt != prim.attributes.end())
			{
				fastgltf::iterateAccessorWithIndex<glm::vec4>(
					asset, asset.accessors[tanIt->accessorIndex],
					[&](glm::vec4 v, size_t i) { data.vertices[i].tangent = v; });
			}
			else generate_tangents_mikktspace(data);

			MeshHandle handle = get_mesh_pool().load(data);
			if (!handle.is_valid()) return std::nullopt;

			progress.meshes.push_back(handle);
			return handle;
		}

		void decompose_transform(
			const fastgltf::Node& node, ModelNode& out)
		{
			ENGINE_ZONE_NAME("Decompose Transform");

			if (const auto* trs = std::get_if<
				fastgltf::TRS>(&node.transform))
			{
				out.position = {
					trs->translation[0],
					trs->translation[1],
					trs->translation[2] };
				out.rotation = glm::quat(
					trs->rotation[3], trs->rotation[0],
					trs->rotation[1], trs->rotation[2]);
				out.scale = { trs->scale[0],
					trs->scale[1], trs->scale[2] };
			}
			else if (const auto* mat = std::get_if<
				fastgltf::math::fmat4x4>(&node.transform))
			{
				const glm::mat4 m = glm::make_mat4(mat->data());
				glm::vec3 skew; glm::vec4 perspective;
				glm::decompose(m, out.scale, out.rotation,
					out.position, skew, perspective);
			}
		}
	} // anonymous namespace

	namespace Internal
	{
		std::optional<Model> load_gltf(const fs::path& path,
			ShaderHandle shader)
		{
			ENGINE_ZONE_NAME("Load glTF");

			const fs::path baseDir = path.parent_path();

			auto bufferResult = fastgltf::GltfDataBuffer::FromPath(path);
			if (bufferResult.error() != fastgltf::Error::None)
			{
				Logger::Internal::error("Failed to read glTF file '{}'", path.string());
				return std::nullopt;
			}

			constexpr fastgltf::Options options{
				fastgltf::Options::LoadExternalBuffers |
				fastgltf::Options::LoadExternalImages |
				fastgltf::Options::DecomposeNodeMatrices |
				fastgltf::Options::LoadGLBBuffers };

			fastgltf::Parser parser;
			auto assetResult = parser.loadGltf(
				bufferResult.get(), baseDir, options);

			if (assetResult.error() != fastgltf::Error::None)
			{
				Logger::Internal::error(
					"Failed to parse glTF file '{}': error code {}",
					path.string(), static_cast<i32>(assetResult.error()));
				return std::nullopt;
			}

			if (!get_shader_pool().is_valid(shader))
				shader = get_default_shader();

			fastgltf::Asset& asset = assetResult.get();

			LoadProgress progress;

			/* --- Materials --- */
			std::vector<MaterialHandle> matHandles;
			size_t matsSize = asset.materials.size();
			matHandles.reserve(matsSize);
			for (size_t i = 0; i < matsSize; ++i)
			{
				matHandles.push_back(build_material(
					asset, i, baseDir, shader, progress));
			}

			/* --- Meshes --- */
			std::unordered_map<u64, MeshHandle> meshHandles;
			for (size_t midx = 0; midx < asset.meshes.size(); ++midx)
			{
				const fastgltf::Mesh& mesh = asset.meshes[midx];
				for (size_t pidx = 0; pidx < mesh.primitives.size(); ++pidx)
				{
					auto handle = build_mesh(asset, mesh.primitives[pidx], progress);
					if (!handle.has_value())
					{
						progress.rollback();
						return std::nullopt;
					}
					meshHandles[(static_cast<u64>(midx) << 32) | pidx] = *handle;
				}
			}

			/* --- Model --- */
			Model model;
			model.nodes.reserve(asset.nodes.size());

			// First pass: on ModelNode per glTF node, transform + primitives
			for (size_t i = 0; i < asset.nodes.size(); ++i)
			{
				const fastgltf::Node& srcNode = asset.nodes[i];

				ModelNode node;
				node.name = srcNode.name;
				decompose_transform(srcNode, node);

				if (srcNode.meshIndex.has_value())
				{
					const fastgltf::Mesh& mesh = asset.meshes[*srcNode.meshIndex];
					node.primitives.reserve(mesh.primitives.size());

					for (size_t pidx = 0; pidx < mesh.primitives.size(); ++pidx)
					{
						const auto& prim = mesh.primitives[pidx];
						const u64 key = (static_cast<u64>(*srcNode.meshIndex) << 32) | pidx;

						ModelPrimitive p;
						p.mesh = meshHandles.at(key);
						p.material = prim.materialIndex.has_value() ?
							matHandles[*prim.materialIndex] : get_default_material();

						if (asset.materials.empty())
							get_material_pool().try_get(p.material)->shader = shader;

						node.primitives.push_back(p);
					}
				}

				model.nodes.push_back(std::move(node));
			}

			// Second pass: fill in parent by walking each node's children
			for (size_t i = 0; i < asset.nodes.size(); ++i)
			{
				for (size_t cidx : asset.nodes[i].children)
					model.nodes[cidx].parent = static_cast<u32>(i);
			}

			return model;
		}
	}
}