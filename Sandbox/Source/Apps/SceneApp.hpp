#pragma once

#include <WinkEngine/Core.hpp>
#include <WinkEngine/ECS.hpp>
#include <WinkEngine/GFX.hpp>

using namespace Wink;

class SandboxApp : public Application
{
public:
	void on_init()
	{
		register_inputs();

		auto* scene = ECS::create_scene("Main Scene");

		/* --- Camera Creation --- */
		mCam.camE = scene->spawn();
		mCam.camE.add<ECS::TransformComponent>().position.z = 5.0f;
		mCam.camE.add<ECS::CameraComponent>(mCam.cam);

		/* --- Sun Creation --- */
		auto sun = scene->spawn();
		auto& l = sun.add<ECS::DirLightComponent>();
		l.dirLight.direction = { -1.0f, -1.0f, -0.5f };
		l.dirLight.intensity = 10.0f;

		/* --- Model Loading --- */
		{
			APP_ZONE_NAME("Model Loading");

			{
				APP_ZONE_NAME("Model Load 'SciFi Radar'");
				auto e = scene->wrap(ECS::instantiate_model(load_model("SciFiRadar")));
				auto& t = e.get<ECS::TransformComponent>();
				t.position.x -= 5.0f;
				t.rotate_pitch(90.0f);
				t.scale /= 60.0f;
			}
			{
				APP_ZONE_NAME("Model Load 'Damaged Helmet'");
				auto e = scene->wrap(ECS::instantiate_model(load_model("DamagedHelmet")));
			}
			{
				APP_ZONE_NAME("Model Load 'Guitar Backpack'");
				auto e = scene->wrap(ECS::instantiate_model(load_model("LaFerrari")));
				auto& t = e.get<ECS::TransformComponent>();
				t.position.x += 5.0f;
				t.scale *= 1.5f;
			}
		}

		/* --- IBL Setup --- */
		{
			auto& texPool = GFX::Resource::get_texture_pool();
			auto& cubemapPool = GFX::Resource::get_cubemap_pool();

			auto hdr = texPool.decode(RES_PATH / "HDRIs" /
				"kloofendal_48d_partly_cloudy_puresky_4k.hdr");
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
	}

private:
	GFX::Resource::ModelHandle load_model(const fs::path& path,
		GFX::Resource::ShaderHandle shader = {}) const
	{
		const fs::path modelsPath = RES_PATH / "Models";
		auto& modelPool = GFX::Resource::get_model_pool();

		const GFX::Resource::ModelHandle handle =
			modelPool.load(modelsPath / path, shader);
		if (!modelPool.is_valid(handle))
		{
			Logger::warn("Failed to load model from: '{}'",
				fs::absolute(modelsPath / path).string());
		}

		return handle;
	}

	void register_inputs()
	{
		subscribe([this](const WindowResizeEvent& e) {
			GFX::resize(e.width, e.height);
			mCam.cam.aspectRatio = static_cast<float>(e.width) / e.height;
			});

		subscribe([this](const KeyPressEvent& e) {
			using namespace Input;
			if ((e.key == Key::Enter && (e.mods & Mod::Alt)) ||
				e.key == Key::F11)
			{
				Window::toggle_fullscreen();
			}
			});

		subscribe([](const MouseButtonPressEvent& e) {
			using namespace Input;
			if (e.button == MouseButton::Right)
				Window::toggle_cursor_lock();
			});

		subscribe([this](const MouseMoveEvent& e) {
			if (Window::get_state().cursorLocked)
				mCam.cam.look(-e.deltaX, e.deltaY,
					0.0f, mCam.settings.LOOK_SENS);
			});

		subscribe([this](const MouseScrollEvent& e) {
			if (Window::get_state().cursorLocked)
			{
				mCam.cam.fov -= e.offsetY;
				mCam.cam.fov = std::clamp(mCam.cam.fov, 10.0f, 100.0f);
			}
			});
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
	};

private:
	const fs::path PROJ_PATH = fs::path("..") / ".." / ".." / "..";
	const fs::path RES_PATH = PROJ_PATH / "Sandbox" / "Resources";

	Camera mCam;
};