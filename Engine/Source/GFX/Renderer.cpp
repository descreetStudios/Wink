#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Renderer.hpp>

#include <WinkEngine/ECS/Scene.hpp>
#include <WinkEngine/ECS/Components/TransformComponent.hpp>
#include <WinkEngine/ECS/Components/RenderObjectComponent.hpp>
#include <WinkEngine/ECS/Systems/TransformSystem.hpp>

#include <WinkEngine/Core/Logger.hpp>
#include <GLFW/glfw3.h>

namespace Wink::GFX
{
	namespace
	{
		Resource::MeshPool gMeshPool;
		Resource::ShaderPool gShaderPool;
		Resource::TexturePool gTexturePool;
		Resource::MaterialPool gMaterialPool;
		Resource::ModelPool gModelPool;
	} // anonymous namespace

	bool init()
	{
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			Logger::critical("Failed to initialize GLAD");
			return false;
		}

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		return true;
	}

	void render()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		auto scene = ECS::get_active_scene();
		static bool sceneWarn = true;
		if (!scene)
		{
			if (sceneWarn)
			{
				Logger::warn("No scene found to render from");
				sceneWarn = false;
			}
			return;
		}

		for (auto&& [id, tC, roC] :
			scene->view<ECS::TransformComponent,
			ECS::RenderObjectComponent>())
		{
			auto e = scene->wrap(id);
			if (tC.dirty)
				ECS::update_world_transform(*scene, id);
		}
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
		MeshPool& get_mesh_pool() noexcept
		{
			return gMeshPool;
		}

		ShaderPool& get_shader_pool() noexcept
		{
			return gShaderPool;
		}

		TexturePool& get_texture_pool() noexcept
		{
			return gTexturePool;
		}

		MaterialPool& get_material_pool() noexcept
		{
			return gMaterialPool;
		}

		ModelPool& get_model_pool() noexcept
		{
			return gModelPool;
		}

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