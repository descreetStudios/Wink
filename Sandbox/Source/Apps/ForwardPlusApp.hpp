#pragma once

#include <WinkEngine/Core.hpp>
#include <WinkEngine/ECS.hpp>
#include <WinkEngine/GFX.hpp>

#define SPONZA 1

using namespace Wink;

class SandboxApp : public Application
{
public:
	Window::Config get_window_config() const override
	{
		Window::Config cfg;
		cfg.vsync = true;
		return cfg;
	}

	void on_init() override
	{
		register_inputs();

		auto* scene = ECS::create_scene("Main Scene");

		/* --- Camera Creation --- */
		mCam.camE = scene->spawn();
		mCam.camE.add<ECS::CameraComponent>(mCam.cam);
		auto& t = mCam.camE.add<ECS::TransformComponent>();
#if SPONZA
		t.position = { 0.0f, -1.5f, 0.0f };
		mCam.cam.yaw = 90.0f;
#else
		t.position = { -5.0f, 0.0f, -8.0f };
		mCam.cam.yaw = -120.0f;
#endif

		/* --- Sun Creation --- */
		mSun = scene->spawn();
		auto& l = mSun.add<ECS::DirLightComponent>();
		l.dirLight.direction = { 0.0f, -1.0f, 0.0f };
		l.dirLight.intensity = 1.0f;

		/* --- Random Point & Spot Lights --- */
#if SPONZA
		spawn_random_lights(scene, 1000, 800,
			{ -15.0f, -3.0f, -5.0f }, { 15.0f, 6.0f, 5.0f });
#else
		spawn_random_lights(scene, 100, 100);
#endif

		/* --- Model Loading --- */
		{
			APP_ZONE_NAME("Model Loading");

			const fs::path shaders = PROJ_PATH / "Engine" / "Source" / "GFX" / "Shaders";
			GFX::RES::ShaderHandle shader = GFX::RES::get_shader_pool().load(
				std::vector<GFX::ShaderFile>{
					{ GFX::ShaderType::Vertex, shaders / "DefaultVS.glsl" },
					{ GFX::ShaderType::Fragment, shaders / "DefaultFS.glsl" },
			});

			{
#if SPONZA
				APP_ZONE_NAME("Model Load 'Sponza'");
				auto e = scene->wrap(ECS::instantiate_model(load_model(
					"Sponza", shader)));
				e.get<ECS::TransformComponent>().position = { 0.6f, -3.0f, 0.3f };

#else
				APP_ZONE_NAME("Model Load 'San Miguel'");
				auto e = scene->wrap(ECS::instantiate_model(load_model(
					fs::path("san-miguel") / "san-miguel.gltf", shader)));
				e.get<ECS::TransformComponent>().position = { -15.0f, -1.5f, -6.5f };
#endif
			}
		}

		/* --- IBL Setup --- */
		{
			auto& texPool = GFX::RES::get_texture_pool();
			auto& cubemapPool = GFX::RES::get_cubemap_pool();

			auto hdr = texPool.decode(RES_PATH / "HDRIs" /
				"shanghai_bund_4k.hdr");
			auto env = cubemapPool.hdr_to_cubemap(hdr);
			auto e = scene->spawn();
			auto& ibl = e.add<ECS::IBLComponent>();
			ibl.iblData = { env,
				GFX::IBL::bake_irradiance_map(env),
				GFX::IBL::bake_prefiltered_env_map(env)
			};
		}
	}

	void on_update(double dt)
	{
		const float fdt = static_cast<float>(dt);

		/* --- Update Camera --- */
		{
			using namespace Input;

			float speed = mCam.settings.MOVE_SPEED;
			if (Input::is_key_down(Key::LeftShift))
				speed += 7.0f;

			auto& camT = mCam.camE.get<ECS::TransformComponent>();

			const float move = fdt * speed;
			if (Input::is_key_down(Key::W))
				camT.translate(mCam.cam.get_forward() * move);
			if (Input::is_key_down(Key::S))
				camT.translate(-mCam.cam.get_forward() * move);
			if (Input::is_key_down(Key::A))
				camT.translate(-mCam.cam.get_right() * move);
			if (Input::is_key_down(Key::D))
				camT.translate(mCam.cam.get_right() * move);
			if (Input::is_key_down(Key::E))
				camT.translate({ 0.0f, move, 0.0f });
			if (Input::is_key_down(Key::Q))
				camT.translate({ 0.0f, -move, 0.0f });
		}

		/* --- Animate Sun Direction --- */
		{
			static float time = 0.0f;
			time += fdt;

			const float speed = 0.3f;
			const float yTilt = -2.0f;

			float x = std::cos(time * speed);
			float z = std::sin(time * speed);
			glm::vec3 orbitDir = glm::normalize(glm::vec3(x, yTilt, z));

			auto& sun = mSun.get<ECS::DirLightComponent>();
			sun.dirLight.direction = orbitDir;
		}
	}

private:
	void register_inputs()
	{
		subscribe([this](const WindowResizeEvent& e) {
			GFX::resize(e.width, e.height);
			mCam.cam.aspectRatio = static_cast<float>(e.width) / e.height;
			mCam.ignoreMove = true;
			});

		subscribe([this](const KeyPressEvent& e) {
			using namespace Input;
			if ((e.key == Key::Enter && (e.mods & Mod::Alt)) ||
				e.key == Key::F11)
			{
				Window::toggle_fullscreen();
			}
			if (e.key == Key::Num1)
			{
				mSettings.postProcessSettings.msaa.enabled =
					!mSettings.postProcessSettings.msaa.enabled;
				GFX::apply_settings(mSettings);
			}
			});

		subscribe([](const MouseButtonPressEvent& e) {
			using namespace Input;
			if (e.button == MouseButton::Right)
				Window::toggle_cursor_lock();
			});

		subscribe([this](const MouseMoveEvent& e) {
			if (mCam.ignoreMove) { mCam.ignoreMove = false; return; }
			if (Window::get_state().cursorLocked)
			{
				mCam.cam.look(-e.deltaX, e.deltaY,
					0.0f, mCam.settings.LOOK_SENS);
			}
			});

		subscribe([this](const MouseScrollEvent& e) {
			if (Window::get_state().cursorLocked)
			{
				mCam.cam.fov -= e.offsetY;
				mCam.cam.fov = std::clamp(mCam.cam.fov, 10.0f, 100.0f);
			}
			});
	}

	GFX::RES::ModelHandle load_model(const fs::path& path,
		GFX::RES::ShaderHandle shader = {}) const
	{
		const fs::path modelsPath = RES_PATH / "Models";
		auto& modelPool = GFX::RES::get_model_pool();

		const GFX::RES::ModelHandle handle =
			modelPool.load(modelsPath / path, shader);

		if (!modelPool.is_valid(handle))
		{
			Logger::warn("Failed to load model from: '{}'",
				fs::absolute(modelsPath / path).string());
		}

		return handle;
	}

	void spawn_random_lights(ECS::Scene* scene,
		u32 numPoint = 100, u32 numSpot = 100,
		glm::vec3 posMin = { -7.0f, 0.5f, -7.0f },
		glm::vec3 posMax = { 7.0f, 5.0f, 7.0f },
		float radiusMin = 0.5f, float radiusMax = 1.0,
		u32 seed = 1338) const
	{
		std::mt19937 rng(seed);
		std::uniform_real_distribution<float> ux(posMin.x, posMax.x);
		std::uniform_real_distribution<float> uy(posMin.y, posMax.y);
		std::uniform_real_distribution<float> uz(posMin.z, posMax.z);
		std::uniform_real_distribution<float> ur(radiusMin, radiusMax);
		std::uniform_real_distribution<float> uc(0.0f, 1.0f);
		std::uniform_real_distribution<float> uangle(10.0f, 45.0f);

		for (u32 i = 0; i < numPoint; ++i)
		{
			auto e = scene->spawn();
			auto& t = e.add<ECS::TransformComponent>();
			t.position = { ux(rng), uy(rng), uz(rng) };
			auto& pl = e.add<ECS::PointLightComponent>();
			pl.pointLight.color = { uc(rng), uc(rng), uc(rng) };
			pl.pointLight.radius = ur(rng);
			pl.pointLight.intensity = 5.0f;
		}

		auto random_spot_direction = [&](std::mt19937& rng)
			{
				std::uniform_real_distribution<float> u(0.0f, 1.0f);

				float azimuth = u(rng) * glm::two_pi<float>();
				float elevation = glm::radians(20.0f + u(rng) * 60.0f);

				float x = std::cos(azimuth) * std::sin(elevation);
				float y = -std::cos(elevation);
				float z = std::sin(azimuth) * std::sin(elevation);

				return glm::normalize(glm::vec3(x, y, z));
			};

		for (u32 i = 0; i < numSpot; ++i)
		{
			auto e = scene->spawn();
			auto& t = e.add<ECS::TransformComponent>();
			t.position = { ux(rng), uy(rng), uz(rng) };
			glm::vec3 dir = random_spot_direction(rng);
			auto& sl = e.add<ECS::SpotLightComponent>();
			sl.spotLight.color = { uc(rng), uc(rng), uc(rng) };
			sl.spotLight.direction = dir;
			sl.spotLight.range = 3.0f;
			sl.spotLight.intensity = 5.0f;
			sl.spotLight.innerCutoff = glm::cos(glm::radians(uangle(rng) * 0.5f));
			sl.spotLight.outerCutoff = glm::cos(glm::radians(uangle(rng)));
		}
	}

private:
	struct CameraSettings
	{
		const float LOOK_SENS = 0.1f;
		const float MOVE_SPEED = 5.0f;
	};

	struct Camera
	{
		GFX::Camera cam;
		CameraSettings settings;
		ECS::Entity camE;
		bool ignoreMove = false;
	};

private:
	const fs::path PROJ_PATH = fs::path("..") / ".." / ".." / "..";
	const fs::path RES_PATH = PROJ_PATH / "Sandbox" / "Resources";

	GFX::Settings mSettings;
	Camera mCam;
	
	ECS::Entity mSun;
};