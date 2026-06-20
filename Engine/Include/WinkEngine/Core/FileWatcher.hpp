#pragma once

namespace Wink
{
	class FileWatcher
	{
	public:
		using Callback = std::function<void(const fs::path&)>;

		void watch(const fs::path& path, Callback callback);
		void unwatch(const fs::path& path);

		void poll();

	private:
		struct WatchedFile
		{
			fs::path path;
			fs::file_time_type lastWriteTime;
			Callback callback;
		};

		std::unordered_map<std::string, WatchedFile> mWatched;
	};
}