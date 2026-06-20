#include <WinkEngine/pch.hpp>
#include <WinkEngine/Core/FileWatcher.hpp>

namespace Wink
{
	void FileWatcher::watch(const fs::path& path, Callback callback)
	{
		std::error_code ec;
		const auto writeTime = fs::last_write_time(path, ec);

		mWatched[path.string()] = WatchedFile{
			.path = path,
			.lastWriteTime = ec ? fs::file_time_type{} : writeTime,
			.callback = std::move(callback)
		};
	}

	void FileWatcher::unwatch(const fs::path& path)
	{
		mWatched.erase(path.string());
	}

	void FileWatcher::poll()
	{
		for (auto& [key, watched] : mWatched)
		{
			std::error_code ec;
			const auto writeTime = fs::last_write_time(watched.path, ec);
			if (ec) continue;

			if (writeTime != watched.lastWriteTime)
			{
				watched.lastWriteTime = writeTime;
				watched.callback(watched.path);
			}
		}
	}
}