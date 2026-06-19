#include <WinkEngine/pch.hpp>
#include <WinkEngine/Core/Time.hpp>

namespace Wink::Time
{
	using Clock = std::chrono::steady_clock;
	using TimePoint = Clock::time_point;
	using Duration = std::chrono::duration<double>;

	namespace
	{
		constexpr double FIXED_STEP = 1.0 / 60.0;

		TimePoint gLast;
		double gDelta = 0.0;
		double gAccumulator = 0.0;
		u64 gFrame = 0;
	} // anonymous namespace

	void init()
	{
		gLast = Clock::now();
	}

	void tick()
	{
		auto now = Clock::now();
		Duration diff = now - gLast;
		gLast = now;

		gDelta = diff.count();

		if (gDelta > 0.25)
			gDelta = 0.25;

		gAccumulator += gDelta;
		gFrame++;
	}

	void consume_fixed_step()
	{
		gAccumulator -= FIXED_STEP;
	}

	double get_delta() noexcept
	{
		return gDelta;
	}

	double get_fixed_step() noexcept
	{
		return FIXED_STEP;
	}

	double get_alpha() noexcept
	{
		if (FIXED_STEP == 0.0) return 0.0;
		return gAccumulator / FIXED_STEP;
	}

	double get_frame_number() noexcept
	{
		return gFrame;
	}

	bool should_simulate() noexcept
	{
		return gAccumulator >= FIXED_STEP;
	}
}