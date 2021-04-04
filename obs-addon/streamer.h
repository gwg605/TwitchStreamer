#pragma once

#include "streamer_types.h"
#include <string>
#include <memory>
#include "libobs/obs.hpp"

namespace twitch_streamer {

class CStreamer {
public:
	using Ptr = std::shared_ptr<CStreamer>;

protected:
	/// Indicates streaming stared or not (TODO: replace on checking actual state of OBS)
	bool m_Started{ false };
	/// Streamin output module
	OBSOutput m_StreamingOutput;

public:
	/**
	 * 	Initialize OBS environment
	 */
	Error Init();

	/**
	 *	Check if streaming is already started
	 */
	bool IsStarted() const;

	/**
	 *	Start streaming
	 *	@param twitch_key - streaming Twitch key
	 */
	Error Start( const std::string& twitch_key );

	/**
	 *	Stop streaming
	 *	@note if streaming is already stopped then do nothing
	 */
	void Stop();

	/**
	 * 	Shutdown OBS environment
	 */
	void Shutdown();

	/**
	 * 	Get version of OBS addon
	 */
	static std::string GetVersion();

	/**
	 *	Get OBS core version
	 */
	static std::string GetOBSVersion();
};

}  // namespace twitch_streamer
