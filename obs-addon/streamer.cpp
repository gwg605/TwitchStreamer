#include "streamer.h"
#include <Windows.h>
#include <filesystem>

namespace twitch_streamer {

static void LoadModules( const char* data_path, const char* bin_path ) {
	std::filesystem::path modules_search{ data_path };

	if( !std::filesystem::is_directory( modules_search ) ) {
		blog( LOG_INFO, "Plugin data folder '%s' does not exist", modules_search.u8string().c_str() );
		return;
	}

	for( auto& entry : std::filesystem::directory_iterator( modules_search ) ) {
		std::string module_data_path = entry.path().u8string();

		std::string module_name = entry.path().filename().u8string();
		std::string module_library = module_name + ".dll";
		std::filesystem::path module_bin{ bin_path };
		module_bin += module_library;

		obs_module_t* module;
		int res = obs_open_module( &module, module_bin.u8string().c_str(), module_data_path.c_str() );
		if( res == MODULE_SUCCESS ) {
			obs_init_module( module );
		} else {
			blog( LOG_ERROR, "Unable to load '%s' module (bin=%s). Error=%i", module_name.c_str(),
				  module_bin.u8string().c_str(), res );
		}
	}
}

Error CStreamer::Init() {
	if( !obs_startup( "en-us", NULL, NULL ) ) {
		blog( LOG_ERROR, "obs_startup() failed" );
		return Error::InitializationFailed;
	}

#ifdef _DEBUG
	obs_add_data_path( "../obs/build/rundir/Debug/data/libobs/" );

	LoadModules( "../obs/build/rundir/Debug/data/obs-plugins/", "../obs/build/rundir/Debug/obs-plugins/64bit/" );
#else
	obs_add_data_path( "../obs/build/rundir/Release/data/libobs/" );

	LoadModules( "../obs/build/rundir/Release/data/obs-plugins/", "../obs/build/rundir/Release/obs-plugins/64bit/" );
#endif

	obs_post_load_modules();

	char buf[ 4096 ];
	DWORD len = ::GetCurrentDirectoryA( sizeof( buf ), buf );
	buf[ len ] = 0;
	blog( LOG_DEBUG, "Current path=%s", buf );

	len = ::GetModuleFileNameA( NULL, buf, sizeof( buf ) );
	buf[ len ] = 0;
	blog( LOG_DEBUG, "Run path=%s", buf );

	obs_video_info vi;
	vi.graphics_module = "libobs-d3d11";
	vi.fps_num = 60000;
	vi.fps_den = 2000;
	vi.base_width = 1280;
	vi.base_height = 720;
	vi.output_width = 1280;
	vi.output_height = 720;
	vi.output_format = VIDEO_FORMAT_NV12;
	vi.adapter = 0;
	vi.gpu_conversion = true;
	vi.colorspace = VIDEO_CS_709;
	vi.range = VIDEO_RANGE_PARTIAL;
	vi.scale_type = OBS_SCALE_BICUBIC;

	int err = obs_reset_video( &vi );
	if( err != OBS_VIDEO_SUCCESS ) {
		blog( LOG_ERROR, "obs_reset_video() failed, err=%i", err );
		return Error::InitializationFailed;
	}

	obs_audio_info ai;
	ai.samples_per_sec = 48000;
	ai.speakers = SPEAKERS_STEREO;

	if( !obs_reset_audio( &ai ) ) {
		blog( LOG_ERROR, "obs_reset_audio() failed" );
		return Error::InitializationFailed;
	}

	return Error::Ok;
}

void CStreamer::Shutdown() {
	obs_shutdown();
}

bool CStreamer::IsStarted() const {
	return m_Started;
}

const char* GetPropertyListItem( obs_properties_t* properties, const char* prop_name, size_t idx ) {
	obs_property_t* property = obs_properties_get( properties, prop_name );
	if( property == nullptr ) {
		return nullptr;
	}
	obs_property_type type = obs_property_get_type( property );
	if( type != OBS_PROPERTY_LIST && type != OBS_PROPERTY_EDITABLE_LIST ) {
		return nullptr;
	}

	size_t count = obs_property_list_item_count( property );
	if( idx >= count ) {
		return nullptr;
	}

	obs_combo_format format = obs_property_list_format( property );
	if( format != OBS_COMBO_FORMAT_STRING ) {
		return nullptr;
	}

	return obs_property_list_item_string( property, idx );
}

static OBSSource SourceSetup() {
	OBSSource source = obs_source_create( "dshow_input", "Capture", nullptr, nullptr );
	obs_properties_t* source_properties = obs_source_properties( source );
	const char* video_device_id = GetPropertyListItem( source_properties, "video_device_id", 0 );
	const char* audio_device_id = GetPropertyListItem( source_properties, "audio_device_id", 0 );

	OBSData src_settings = obs_source_get_settings( source );
	if( video_device_id != nullptr ) {
		obs_data_set_string( src_settings, "video_device_id", video_device_id );
		obs_data_set_string( src_settings, "resolution", "1280x720" );
	}
	if( audio_device_id != nullptr ) {
		obs_data_set_bool( src_settings, "use_custom_audio_device", true );
		obs_data_set_string( src_settings, "audio_device_id", audio_device_id );
	}
	obs_data_set_string( src_settings, "video_format", "nv12" );
	obs_data_set_string( src_settings, "color_space", "709" );
	obs_data_set_string( src_settings, "color_range", "partial" );

	obs_source_update( source, src_settings );

	obs_set_output_source( 0, source );

	return source;
}

static OBSEncoder VideoEncoderSetup() {
	OBSEncoder video_encoder = obs_video_encoder_create( "obs_x264", "Video Encoder(x264)", nullptr, nullptr );
	video_t* video = obs_get_video();

	obs_encoder_set_video( video_encoder, video );
	OBSData ve_settings = obs_data_create();
	obs_data_set_int( ve_settings, "bitrate", 2400 );
	obs_data_set_string( ve_settings, "rate_control", "CBR" );
	obs_data_set_int( ve_settings, "keyint_sec", 1 );
	obs_encoder_update( video_encoder, ve_settings );

	return video_encoder;
}

static OBSEncoder AudioEncoderSetup() {
	OBSEncoder audio_encoder = obs_audio_encoder_create( "ffmpeg_aac", "Audio Encoder(AAC)", nullptr, 0, nullptr );

	audio_t* audio = obs_get_audio();
	obs_encoder_set_audio( audio_encoder, audio );
	OBSData ae_settings = obs_data_create();
	obs_data_set_int( ae_settings, "bitrate", 64 );
	obs_encoder_update( audio_encoder, ae_settings );

	return audio_encoder;
}

static OBSService ServiceSetup( const std::string& key ) {
	OBSService service = obs_service_create( "rtmp_common", "default_service", nullptr, nullptr );

	OBSData service_settings = obs_data_create();
	obs_data_set_string( service_settings, "key", key.c_str() );
	obs_data_set_string( service_settings, "server", "auto" );
	obs_data_set_string( service_settings, "service", "Twitch" );
	obs_service_update( service, service_settings );

	return service;
}

static const char* s_RequiredModules[] = { "win-dshow", "rtmp-services", "obs-x264", "obs-outputs", "obs-ffmpeg" };

static bool CheckForRequiredModules() {
	for( const auto& name : s_RequiredModules ) {
		obs_module_t* module = obs_get_module( name );
		if( module == nullptr ) {
			blog( LOG_ERROR, "Module '%s' is not found", name );
			return false;
		}
	}

	return true;
}

Error CStreamer::Start( const std::string& twitch_key ) {
	if( m_Started ) {
		blog( LOG_INFO, "Start() - Already started" );
		return Error::Ok;
	}

	if( !CheckForRequiredModules() ) {
		blog( LOG_ERROR, "Start() - Some modules are not loaded" );
		return Error::ModuleNotFound;
	}

	OBSSource source = SourceSetup();
	OBSEncoder video_encoder = VideoEncoderSetup();
	OBSEncoder audio_encoder = AudioEncoderSetup();
	OBSService service = ServiceSetup( twitch_key );

	// start streaming
	m_StreamingOutput = obs_output_create( "rtmp_output", "Streaming output", nullptr, nullptr );
	obs_output_set_video_encoder( m_StreamingOutput, video_encoder );
	obs_output_set_audio_encoder( m_StreamingOutput, audio_encoder, 0 );
	obs_output_set_service( m_StreamingOutput, service );
	obs_output_set_delay( m_StreamingOutput, 0, 0 );
	obs_output_set_reconnect_settings( m_StreamingOutput, 5, 1 );

	m_Started = obs_output_start( m_StreamingOutput );
	if( !m_Started ) {
		blog( LOG_ERROR, "Start() - obs_output_start failed" );
		return Error::UnableToStart;
	}

	blog( LOG_INFO, "Start() - Streaming started" );
	return Error::Ok;
}

void CStreamer::Stop() {
	if( !m_Started ) {
		blog( LOG_INFO, "Stop() - Already stopped" );
		return;
	}

	obs_output_stop( m_StreamingOutput );

	blog( LOG_INFO, "Stop() - Streaming stopped" );
	m_Started = false;
}

std::string CStreamer::GetVersion() {
#ifdef _DEBUG
	return "v1.0.0.0 (Debug)";
#else
	return "v1.0.0.0";
#endif
}

std::string CStreamer::GetOBSVersion() {
	return OBS_VERSION;
}

}  // namespace twitch_streamer
