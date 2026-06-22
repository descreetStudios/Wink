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
		using namespace GFX::Resource;

		subscribe_to_events();

		GFX::set_clear_color({ 0.53f, 0.81f, 0.92f, 1.0f });

		auto* sponzaScene = create_scene("Sponza Scene");
		auto* gameScene = create_scene("A Beautiful Game Scene");

		mCamEntity = sponzaScene->spawn();
		mCamEntity.add<TransformComponent>().position = { 0.0f, 2.0f, 0.0f };
		mCamEntity.add<CameraComponent>(mCam);

		auto& meshPool = get_mesh_pool();
		auto& shaderPool = get_shader_pool();
		auto& texturePool = get_texture_pool();
		auto& materialPool = get_material_pool();
		auto& modelPool = get_model_pool();

		// For hot-reloading default shaders
		const fs::path root = fs::path("..") / ".." / ".." / "..";
		const fs::path shaders = root / "Engine" / "Source" / "GFX" / "Shaders";
		ShaderHandle shader = shaderPool.load(std::vector<GFX::ShaderFile>{
			{ GFX::ShaderType::Vertex, shaders / "default_vs.glsl" },
			{ GFX::ShaderType::Fragment, shaders / "default_fs.glsl" },
		});

		const ModelHandle sponza = load_model(
			modelPool, fs::path("Sponza") / "glTF", shader);
		const EntityID sponzaID = instantiate_model(sponza, sponzaScene);

		const ModelHandle game = load_model(
			modelPool, fs::path("ABeautifulGame") / "glTF", shader);
		const EntityID gameID = instantiate_model(game, gameScene);
		gameScene->wrap(gameID).get<TransformComponent>().multiply_scale(glm::vec3(10.0f));
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
	}

private:
	GFX::Resource::ModelHandle load_model(
		GFX::Resource::ModelPool& modelPool,
		const fs::path& path, GFX::Resource::ShaderHandle shader = {})
	{
		const fs::path root = fs::path("..") / ".." / ".." / "..";
		const fs::path models = root / fs::path("..")
			/ "glTF-Sample-Models" / "2.0";

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
			});

		subscribe([](const MouseButtonPressEvent& e) {
			if (e.button == MouseButton::Right)
				Window::toggle_cursor_lock();
			});

		subscribe([this](const MouseMoveEvent& e) {
			if (Window::get_state().cursorLocked)
				mCam.look(-e.deltaX, e.deltaY, 0.0f, LOOK_SENS);
			});
	}

private:
	const float LOOK_SENS = 0.1f;
	const float MOVE_SPEED = 5.0f;
	GFX::Camera mCam;
	ECS::Entity mCamEntity;
};