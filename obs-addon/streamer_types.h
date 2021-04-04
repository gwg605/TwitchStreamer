#pragma once

namespace twitch_streamer {
/**
 *	List of possible errors
 **/
enum class Error : int {
	/// No error
	Ok = 0,
	/// Something unexpected
	Fatal = -1,
	/// Initialization procedure is failed
	InitializationFailed = -2,
	/// Starting procedure is failed
	UnableToStart = -3,
	/// Required module not found
	ModuleNotFound = -4,
};

}  // namespace twitch_streamer
