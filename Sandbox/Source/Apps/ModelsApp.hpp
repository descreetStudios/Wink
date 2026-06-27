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

		std::random_device rd;
		std::mt19937 gen(rd());

		subscribe_to_events();

		auto* sponzaScene = create_scene("Sponza Scene");
		auto* gameScene = create_scene("A Beautiful Game Scene");

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
		mCamEntity = sponzaScene->spawn();
		auto& camT = mCamEntity.add<TransformComponent>();
		camT.position = { -5.0f, 2.0f, 0.0f };
		mCamEntity.add<CameraComponent>(mCam).camera.yaw = -90.0f;

		// Dir Lights
		mSponzaDirLight = sponzaScene->spawn();
		auto& sdlC = mSponzaDirLight.add<DirLightComponent>();
		sdlC.dirLight.direction = { 0.0f, -1.0f, 0.3f };

		mGameDirLight = gameScene->spawn();
		auto& gdlC = mGameDirLight.add<DirLightComponent>();
		gdlC.dirLight.direction = { 0.0f, -1.0f, -0.3f };

		// Point Lights
		std::uniform_real_distribution<float> distX(-15.0f, 15.0f);
		std::uniform_real_distribution<float> distY(0.5f, 3.0f);
		std::uniform_real_distribution<float> distZ(-5.0f, 5.0f);
		std::uniform_real_distribution<float> distColor(0.1f, 1.0f);
		std::uniform_real_distribution<float> distSpeed(0.5f, 1.5f);

		for (int i = 0; i < 15; ++i)
		{
			glm::vec3 randomColor = { distColor(gen), distColor(gen), distColor(gen) };

			// Sponza Scene Lights
			auto sponzaLight = sponzaScene->spawn();
			auto& sponzaPosT = sponzaLight.add<TransformComponent>();
			sponzaPosT.position = { distX(gen), distY(gen), distZ(gen) };

			auto& sponzaLightC = sponzaLight.add<PointLightComponent>();
			sponzaLightC.pointLight.color = randomColor;
			sponzaLightC.pointLight.radius = 30.0f;
			sponzaLightC.pointLight.intensity = 5.0f;

			// Save tracking data for Sponza light
			mMovingLights.push_back({
				sponzaLight,
				sponzaPosT.position,
				{ distSpeed(gen), distSpeed(gen), distSpeed(gen) }
				});

			// A Beautiful Game Scene Lights
			auto gameLight = gameScene->spawn();
			auto& gamePosT = gameLight.add<TransformComponent>();
			gamePosT.position = { distX(gen), distY(gen), distZ(gen) };

			auto& gameLightC = gameLight.add<PointLightComponent>();
			gameLightC.pointLight.color = randomColor;
			gameLightC.pointLight.radius = 30.0f;
			gameLightC.pointLight.intensity = 5.0f;

			// Save tracking data for Game light
			mMovingLights.push_back({
				gameLight,
				gamePosT.position,
				{ distSpeed(gen), distSpeed(gen), distSpeed(gen) }
				});
		}

		// Spot Lights
		mSponzaSpotLight = sponzaScene->spawn();
		mSponzaSpotLight.add<SpotLightComponent>().spotLight.intensity = 2.0f;

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

		// Skybox
		auto meadowHdr = texturePool.decode(RES_PATH / "HDRIs" / "meadow_2_4k.hdr");
		auto pureskyHdr = texturePool.decode(RES_PATH / "HDRIs" / "autumn_field_puresky_4k.hdr");
		auto studioHdr = texturePool.decode(RES_PATH / "HDRIs" / "monochrome_studio_02_4k.hdr");
		auto meadowCubemap = cubemapPool.hdr_to_cubemap(meadowHdr);
		auto pureskyCubemap = cubemapPool.hdr_to_cubemap(pureskyHdr);
		auto studioCubemap = cubemapPool.hdr_to_cubemap(studioHdr);

		sponzaScene->spawn().add<IBLComponent>().cubemap = pureskyCubemap;
		gameScene->spawn().add<IBLComponent>().cubemap = studioCubemap;
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

		/* --- Dir Lights --- */
		static float angle = 0.0f;
		angle += fdt * 0.5f;

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

		/* --- Spot Lights --- */
		if (mSponzaSpotLight.has<ECS::SpotLightComponent>() &&
			mCamEntity.has<ECS::TransformComponent>())
		{
			auto& sslC = mSponzaSpotLight.get<ECS::SpotLightComponent>();
			auto& camTransform = mCamEntity.get<ECS::TransformComponent>();
			sslC.spotLight.position = camTransform.position;
			sslC.spotLight.direction = mCam.get_forward();
		}

		/* --- Point Lights --- */
		static float totalTime = 0.0f;
		totalTime += fdt;

		const float maxMovementRadius = 1.0f;

		for (auto& ml : mMovingLights)
		{
			if (ml.entity.is_valid() && ml.entity.has<ECS::TransformComponent>())
			{
				auto& t = ml.entity.get<ECS::TransformComponent>();

				float offsetX = glm::sin(totalTime * ml.movementSpeed.x) * maxMovementRadius;
				float offsetY = glm::cos(totalTime * ml.movementSpeed.y) * (maxMovementRadius * 0.3f);
				float offsetZ = glm::sin(totalTime * ml.movementSpeed.z + 1.0f) * maxMovementRadius;

				t.position = ml.initialPosition + glm::vec3(offsetX, offsetY, offsetZ);
			}
		}
	}

	void pre_render(double alpha) override
	{
		GFX::set_config(mGFXConfig);
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
	ECS::Entity mSponzaSpotLight;

	struct MovingLight
	{
		ECS::Entity entity;
		glm::vec3 initialPosition;
		glm::vec3 movementSpeed;
	};
	std::vector<MovingLight> mMovingLights;
};