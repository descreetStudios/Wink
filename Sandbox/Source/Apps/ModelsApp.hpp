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
		using namespace Resource;

		subscribe_to_events();

		auto* sponzaScene = create_scene("Sponza Scene");
		auto* gameScene = create_scene("A Beautiful Game Scene");

		auto& meshPool = get_mesh_pool();
		auto& shaderPool = get_shader_pool();
		auto& texturePool = get_texture_pool();
		auto& materialPool = get_material_pool();
		auto& modelPool = get_model_pool();

		// For hot-reloading default shaders
		const fs::path shaders = PROJ_REL_PATH / "Engine" / "Source" / "GFX" / "Shaders";
		ShaderHandle shader = shaderPool.load(std::vector<ShaderFile>{
			{ ShaderType::Vertex, shaders / "DefaultVS.glsl" },
			{ ShaderType::Fragment, shaders / "DefaultFS.glsl" },
		});

		// Camera
		mCamEntity = sponzaScene->spawn();
		auto& camT = mCamEntity.add<TransformComponent>();
		camT.position = { -5.0f, 2.0f, 0.0f };
		mCamEntity.add<CameraComponent>(mCam);

		// Dir Lights
		mSponzaDirLight = sponzaScene->spawn();
		auto& sdlC = mSponzaDirLight.add<DirLightComponent>();
		sdlC.dirLight.direction = { 0.0f, -1.0f, 0.3f };

		mGameDirLight = gameScene->spawn();
		auto& gdlC = mGameDirLight.add<DirLightComponent>();
		gdlC.dirLight.direction = { 0.0f, -1.0f, -0.3f };

		// Point Lights
		const ModelHandle sphere = load_model(
			modelPool, fs::path("sphere.glb"), shader);
		auto* sphereModel = modelPool.try_get(sphere);
		materialPool.try_get(sphereModel->
			nodes[0].primitives[0].material)->
			params.baseColor = { 1.0f, 0.0f, 0.0f, 1.0f };

		auto spl = sponzaScene->wrap(instantiate_model(sphere, sponzaScene));
		auto& splL = spl.add<PointLightComponent>();
		splL.pointLight.color = { 0.5f, 0.1f, 0.2f };
		splL.pointLight.radius = 50.0f;
		splL.pointLight.intensity = 10.0f;
		auto& splT = spl.get<TransformComponent>();
		splT.position = { 9.0f, 2.0f, 3.2f };
		splT.scale /= 10.0f;

		{
			APP_ZONE_NAME("Asset Loading");
			{
				APP_ZONE_COLOR("Load Model: Sponza", 0x4A90E2);
				const ModelHandle sponza = load_model(
					modelPool, fs::path("Sponza"), shader);
				const EntityID sponzaID = instantiate_model(sponza, sponzaScene);
			}

			{
				APP_ZONE_COLOR("Load Model: A Beautiful Game", 0x2ECC71);
				const ModelHandle game = load_model(
					modelPool, fs::path("ABeautifulGame"), shader);
				const EntityID gameID = instantiate_model(game, gameScene);
				gameScene->wrap(gameID).get<TransformComponent>().multiply_scale(glm::vec3(10.0f));
			}
		}
	}

	void on_update(double dt) override
	{
		using namespace Input;

		const float fdt = static_cast<float>(dt);

		float speed = MOVE_SPEED;
		if (Input::is_key_down(Key::LeftShift))
			speed += 7.0f;

		auto& t = mCamEntity.get<ECS::TransformComponent>();

		const float move = fdt * speed;
		if (Input::is_key_down(Key::W))
			t.translate(mCam.get_forward() * move);
		if (Input::is_key_down(Key::S))
			t.translate(-mCam.get_forward() * move);
		if (Input::is_key_down(Key::A))
			t.translate(-mCam.get_right() * move);
		if (Input::is_key_down(Key::D))
			t.translate(mCam.get_right() * move);
		if (Input::is_key_down(Key::E))
			t.translate({ 0.0f, move, 0.0f });
		if (Input::is_key_down(Key::Q))
			t.translate({ 0.0f, -move, 0.0f });

		static float angle = 0.0f;
		angle += static_cast<float>(dt) * 0.5f;

		ECS::Entity activeLightEntity =
			(mActiveSceneName == "A Beautiful Game Scene")
			? mGameDirLight
			: mSponzaDirLight;

		if (!activeLightEntity.has<ECS::DirLightComponent>()) return;
		auto& light = activeLightEntity.get<ECS::DirLightComponent>();

		light.dirLight.direction = glm::normalize(glm::vec3(
			glm::cos(angle),
			-1.0f,
			glm::sin(angle)
		));
	}

	void on_render(double alpha) override
	{
		using namespace GFX;

		set_config(mGFXConfig);
	}

private:
	GFX::Resource::ModelHandle load_model(
		GFX::Resource::ModelPool& modelPool,
		const fs::path& path, GFX::Resource::ShaderHandle shader = {})
	{
		const fs::path models = RES_PATH / "Models";

		const GFX::Resource::ModelHandle handle =
			modelPool.load(models / path, shader);
		if (!modelPool.is_valid(handle))
		{
			Logger::warn("Failed to load model from: '{}'",
				fs::absolute(models / path).string());
		}

		return handle;
	}

	void switch_scene(std::string_view name)
	{
		using namespace ECS;

		const TransformComponent transform =
			mCamEntity.get<TransformComponent>(); // intended copy

		auto* scene = set_active_scene(name);
		mActiveSceneName = name;

		mCamEntity.destroy();
		mCamEntity = scene->spawn();
		mCamEntity.add<TransformComponent>() = transform;
		mCamEntity.add<CameraComponent>(mCam);
	}

	void subscribe_to_events()
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

			if (e.key == Key::KP1)
				switch_scene("Sponza Scene");
			else if (e.key == Key::KP2)
				switch_scene("A Beautiful Game Scene");

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

	std::string mActiveSceneName;

	ECS::Entity mSponzaDirLight;
	ECS::Entity mGameDirLight;
};