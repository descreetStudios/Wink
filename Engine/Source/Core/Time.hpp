#pragma once

namespace Wink::Time
{
	void init();
	void tick();
	void consume_fixed_step();

	[[nodiscard]] double get_delta() noexcept;
	[[nodiscard]] double get_fixed_step() noexcept;
	[[nodiscard]] double get_alpha() noexcept;
	[[nodiscard]] double get_frame_number() noexcept;
	[[nodiscard]] bool should_simulate() noexcept;
}