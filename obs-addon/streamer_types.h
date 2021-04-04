#pragma once

namespace twitch_streamer {

enum class Error : int {
	Ok = 0,
	Fatal = -1,
	InitializationFailed = -2,
	UnableToStart = -3,
};

}  // namespace twitch_streamer
