#pragma once

#include <WinkEngine/Core.hpp>
#include <WinkEngine/ECS.hpp>
#include <WinkEngine/GFX.hpp>

using namespace Wink;

class SandboxApp : public Application
{
public:
	void on_init() override
	{
		using namespace ECS;
		using namespace GFX;
		using namespace RES;

		register_inputs();

		auto* scene = create_scene("IBL Test Scene");

		auto& meshPool = get_mesh_pool();
		auto& shaderPool = get_shader_pool();
		auto& texturePool = get_texture_pool();
		auto& cubemapPool = get_cubemap_pool();
		auto& materialPool = get_material_pool();
		auto& modelPool = get_model_pool();

		// For hot-reloading default shaders
		const fs::path shaders = PROJ_REL_PATH / "Engine" / "Source" / "GFX" / "Shaders";
		ShaderHandle shader = shaderPool.load(std::vector<ShaderFile>{
			{ ShaderType::Vertex, shaders / "DefaultVS.glsl" },
			{ ShaderType::Fragment, shaders / "DefaultFS.glsl" },
		});

		// Camera
		mCamEntity = scene->spawn();
		auto& camT = mCamEntity.add<TransformComponent>().position = { -0.7f, -0.8f, 14.0f };
		mCamEntity.add<CameraComponent>(mCam);

		// Dir Light
		auto sun = scene->spawn();
		sun.add<DirLightComponent>().dirLight.direction = { -0.5f, -1.0f, 0.8f };

		// Spheres
		const ModelHandle sphereModel = load_model(
			modelPool, fs::path("Sphere.glb"), shader);

		const i32 rows = 11;
		const i32 cols = 11;
		const float spacing = 1.3f;

		for (i32 row = 0; row < rows; ++row)
		{
			float roughness = (rows == 1)
				? 0.0f : (float)row / (float)(rows - 1);
			for (i32 col = 0; col < cols; ++col)
			{
				float metallic = (cols == 1)
					? 0.0f : (float)col / (float)(cols - 1);

				const EntityID sphereID = instantiate_model(sphereModel, scene);

				scene->each_descendant(sphereID, [&](Entity e)
					{
						if (!e.has<RenderObjectComponent>()) return;

						auto& roC = e.get<RenderObjectComponent>();

						MaterialHandle cloned = materialPool.clone(roC.renderObj.material);
						auto* mat = materialPool.try_get(cloned);
						mat->params.roughness = roughness;
						mat->params.metallic = metallic;
						roC.renderObj.material = cloned;

						mat->params.roughness = roughness;
						mat->params.metallic = metallic;
					});

				Entity sphereE = scene->wrap(sphereID);
				float xPos = (col - (cols / 2.0f)) * spacing;
				float yPos = (row - (rows / 2.0f)) * spacing;
				auto& t = sphereE.get<TransformComponent>();
				t.position = { xPos, yPos, 0.0f };
				t.scale /= 2.0f;
			}
		}

		mTestTexture = texturePool.decode(RES_PATH / "HDRIs" / "kloofendal_48d_partly_cloudy_puresky_4k.hdr");
		auto env = cubemapPool.hdr_to_cubemap(mTestTexture);
		auto irr = GFX::IBL::bake_irradiance_map(env);
		auto pref = GFX::IBL::bake_prefiltered_env_map(env);

		auto iblE = scene->spawn();
		auto& iblC = iblE.add<IBLComponent>();
		iblC.iblData = { env, irr, pref };
		iblC.skybox = true;
	}

	void post_render() override
	{
		//GFX::render_fullscreen_texture(GFX::IBL::get_brdf_lut());
	}

	void on_update(double dt) override
	{
		using namespace Input;

		const float fdt = static_cast<float>(dt);

		/* --- Camera --- */
		float speed = MOVE_SPEED;
		if (Input::is_key_down(Key::LeftShift))
			speed += 7.0f;

		auto& camT = mCamEntity.get<ECS::TransformComponent>();

		const float move = fdt * speed;
		if (Input::is_key_down(Key::W))
			camT.translate(mCam.get_forward() * move);
		if (Input::is_key_down(Key::S))
			camT.translate(-mCam.get_forward() * move);
		if (Input::is_key_down(Key::A))
			camT.translate(-mCam.get_right() * move);
		if (Input::is_key_down(Key::D))
			camT.translate(mCam.get_right() * move);
		if (Input::is_key_down(Key::E))
			camT.translate({ 0.0f, move, 0.0f });
		if (Input::is_key_down(Key::Q))
			camT.translate({ 0.0f, -move, 0.0f });
	}

private:
	GFX::RES::ModelHandle load_model(GFX::RES::ModelPool& modelPool,
		const fs::path& path, GFX::RES::ShaderHandle shader = {}) const
	{
		const fs::path models = RES_PATH / "Models";

		const GFX::RES::ModelHandle handle =
			modelPool.load(models / path, shader);
		if (!modelPool.is_valid(handle))
		{
			Logger::warn("Failed to load model from: '{}'",
				fs::absolute(models / path).string());
		}

		return handle;
	}

	void register_inputs()
	{
		using namespace Input;

		subscribe([this](const WindowResizeEvent& e) {
			GFX::resize(e.width, e.height);
			mCam.aspectRatio = static_cast<float>(e.width) / e.height;
			});

		subscribe([this](const KeyPressEvent& e) {
			if ((e.key == Key::Enter && (e.mods & Mod::Alt)) ||
				e.key == Key::F11)
			{
				Window::toggle_fullscreen();
			}

			if (e.key == Key::V)
				mGFXConfig.polygonMode = mGFXConfig.polygonMode ==
				GL_FILL ? GL_LINE : GL_FILL;
			});

		subscribe([](const MouseButtonPressEvent& e) {
			if (e.button == MouseButton::Right)
				Window::toggle_cursor_lock();
			});

		subscribe([this](const MouseMoveEvent& e) {
			if (Window::get_state().cursorLocked)
				mCam.look(-e.deltaX, e.deltaY, 0.0f, LOOK_SENS);
			});

		subscribe([this](const MouseScrollEvent& e) {
			if (Window::get_state().cursorLocked)
			{
				mCam.fov -= e.offsetY;
				mCam.fov = std::clamp(mCam.fov, 10.0f, 100.0f);
			}
			});
	}

private:
	const fs::path PROJ_REL_PATH = fs::path("..") / ".." / ".." / "..";
	const fs::path RES_PATH = PROJ_REL_PATH / "Sandbox" / "Resources";

	GFX::Configuration mGFXConfig;

	const float LOOK_SENS = 0.1f;
	const float MOVE_SPEED = 5.0f;
	GFX::Camera mCam;
	ECS::Entity mCamEntity;

	GFX::RES::TextureHandle mTestTexture;
};