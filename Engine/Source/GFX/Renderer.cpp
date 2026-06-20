#include <WinkEngine/pch.hpp>
#include <WinkEngine/GFX/Renderer.hpp>
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

		void clear_all_resources() noexcept
		{
			gShaderPool.clear();
			gTexturePool.clear();
		}
	}
}