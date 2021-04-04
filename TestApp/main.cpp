#include <stdlib.h>
#include <filesystem>
#include "obs.hpp"
#include "util/platform.h"
#include <Windows.h>
#include <conio.h>

void DumpPropertyList( const char* prefix, const char* str_type, obs_property_t* property, obs_data_t* settings ) {
	obs_combo_type type = obs_property_list_type( property );
	obs_combo_format format = obs_property_list_format( property );
	size_t count = obs_property_list_item_count( property );

	const char* name = obs_property_name( property );

	switch( format ) {
	case OBS_COMBO_FORMAT_INT:
		blog( LOG_INFO, "%s: %s:%s=%lli", prefix, name, str_type, obs_data_get_int( settings, name ) );
		break;

	case OBS_COMBO_FORMAT_FLOAT:
		blog( LOG_INFO, "%s: %s:%s=%lf", prefix, name, str_type, obs_data_get_double( settings, name ) );
		break;

	case OBS_COMBO_FORMAT_STRING:
		blog( LOG_INFO, "%s: %s:%s=%s", prefix, name, str_type, obs_data_get_string( settings, name ) );
		break;

	default:
		blog( LOG_INFO, "%s: %s:%s=<invalid combo format>", prefix, name, str_type );
		break;
	}

	for( size_t i = 0; i < count; i++ ) {
		const char* name = obs_property_list_item_name( property, i );

		switch( format ) {
		case OBS_COMBO_FORMAT_INT:
			blog( LOG_INFO, "%s:\t[%zu]%s=%lli", prefix, i, name, obs_property_list_item_int( property, i ) );
			break;

		case OBS_COMBO_FORMAT_FLOAT:
			blog( LOG_INFO, "%s:\t[%zu]%s=%lf", prefix, i, name, obs_property_list_item_float( property, i ) );
			break;

		case OBS_COMBO_FORMAT_STRING:
			blog( LOG_INFO, "%s:\t[%zu]%s=%s", prefix, i, name, obs_property_list_item_string( property, i ) );
			break;

		default:
			blog( LOG_INFO, "%s:\t[%zu]%s=<invalid combo format>", prefix, i, name );
			break;
		}
	}
}

void DumpProperty( const char* prefix, obs_property_t* property, obs_data_t* settings ) {
	const char* name = obs_property_name( property );
	obs_property_type type = obs_property_get_type( property );
	switch( type ) {
	case OBS_PROPERTY_INVALID:
		blog( LOG_INFO, "%s: %s:Unknown", prefix, name );
		return;
	case OBS_PROPERTY_BOOL:
		blog( LOG_INFO, "%s: %s:Bool=%u", prefix, name, obs_data_get_bool( settings, name ) );
		break;
	case OBS_PROPERTY_INT:
		blog( LOG_INFO, "%s: %s:Int=%lli", prefix, name, obs_data_get_int( settings, name ) );
		break;
	case OBS_PROPERTY_FLOAT:
		blog( LOG_INFO, "%s: %s:Float=%lf", prefix, name, obs_data_get_double( settings, name ) );
		break;
	case OBS_PROPERTY_TEXT:
		blog( LOG_INFO, "%s: %s:Text=%s", prefix, name, obs_data_get_string( settings, name ) );
		break;
	case OBS_PROPERTY_LIST:
		DumpPropertyList( prefix, "List", property, settings );
		break;
	case OBS_PROPERTY_BUTTON:
		blog( LOG_INFO, "%s: %s:Button=%s", prefix, name, obs_data_get_string( settings, name ) );
		break;
	case OBS_PROPERTY_COLOR:
		blog( LOG_INFO, "%s: %s:Color:", prefix, name );
		break;
	case OBS_PROPERTY_FONT:
		blog( LOG_INFO, "%s: %s:Font:", prefix, name );
		break;
	case OBS_PROPERTY_PATH:
		blog( LOG_INFO, "%s: %s:Path=%s", prefix, name, obs_data_get_string( settings, name ) );
		break;
	case OBS_PROPERTY_EDITABLE_LIST:
		DumpPropertyList( prefix, "EditableList", property, settings );
		break;
	case OBS_PROPERTY_FRAME_RATE:
		blog( LOG_INFO, "%s: %s:FrameRate:", prefix, name );
		break;
	case OBS_PROPERTY_GROUP:
		blog( LOG_INFO, "%s: %s:Group:", prefix, name );
		break;
	case OBS_PROPERTY_COLOR_ALPHA:
		blog( LOG_INFO, "%s: %s:ColorAlpha:", prefix, name );
		break;
	}
}

void DumpProperties( const char* prefix, obs_properties_t* properties, obs_data_t* settings ) {
	obs_property_t* property = obs_properties_first( properties );
	while( property != nullptr ) {
		DumpProperty( prefix, property, settings );

		obs_property_next( &property );
	}
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

void load_modules( const char* data_path, const char* bin_path ) {
	std::filesystem::path modules_search{ data_path };

	if( !std::filesystem::is_directory( modules_search ) ) {
		blog( LOG_INFO, "Plugin data folder '%s' does not exist", modules_search.u8string().c_str() );
		return;
	}

	for( auto& entry : std::filesystem::directory_iterator( modules_search ) ) {
		std::string module_data_path = entry.path().u8string();
		//blog( LOG_INFO, "\t%s", module_data_path.c_str() );

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

void mc(void* param, obs_module_t* module) {
	obs_get_module_name( module );
	blog( LOG_INFO, "Module: %s, Lib=%s", obs_get_module_name( module ), obs_get_module_lib( module ) );
}

static const char* s_RequiredModules[] = { "win-dshow", "rtmp-services", "obs-x264", "obs-outputs", "obs-ffmpeg" };

static bool CheckForRequiredModules() {
	for( const auto& name : s_RequiredModules ) {
		obs_module_t* module = obs_get_module( name );
		if( module == nullptr ) {
			return false;
		}
	}
	
	return true;
}

int main( int argc, char* argv[] ) {
	int result = -1;

	if( obs_startup( "en-us", NULL, NULL ) ) {

#ifdef _DEBUG
		obs_add_data_path( "../obs/build/rundir/Debug/data/libobs/" );

		load_modules( "../obs/build/rundir/Debug/data/obs-plugins/",
					  "../obs/build/rundir/Debug/obs-plugins/64bit/" );
#else
		obs_add_data_path( "../obs/build/rundir/Release/data/libobs/" );

		load_modules( "../obs/build/rundir/Release/data/obs-plugins/",
					  "../obs/build/rundir/Release/obs-plugins/64bit/" );
#endif

		obs_post_load_modules();

		CheckForRequiredModules();

		char buf[ 4096 ];
		DWORD len = ::GetCurrentDirectoryA( sizeof( buf ), buf );
		buf[ len ] = 0;
		blog( LOG_INFO, "Current path=%s", buf );

		len = ::GetModuleFileNameA( NULL, buf, sizeof( buf ) );
		buf[ len ] = 0;
		blog( LOG_INFO, "Run path=%s", buf );

		const char* val;
		int i = 0;

		while( obs_enum_source_types( i++, &val ) ) {
			blog( LOG_INFO, "Source[%i]=%s", i, val );
		}

		i = 0;
		while( obs_enum_encoder_types( i++, &val ) ) {
			blog( LOG_INFO, "Encoder[%i]=%s", i, val );
		}

		i = 0;
		while( obs_enum_output_types( i++, &val ) ) {
			blog( LOG_INFO, "Output[%i]=%s", i, val );
		}

		i = 0;
		while( obs_enum_service_types( i++, &val ) ) {
			blog( LOG_INFO, "Service[%i]=%s", i, val );
		}

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
		if( err == OBS_VIDEO_SUCCESS ) {

			obs_audio_info ai;
			ai.samples_per_sec = 48000;
			ai.speakers = SPEAKERS_STEREO;

			if( obs_reset_audio( &ai ) ) {
				OBSService service = obs_service_create( "rtmp_common", "default_service", nullptr, nullptr );
				OBSData service_settings = obs_service_get_settings( service );
				obs_properties_t* service_properties = obs_service_properties( service );
				blog( LOG_INFO, "===================================================" );
				blog( LOG_INFO, "Dump 'rtmp_common' service:" );
				blog( LOG_INFO, "---------------------------------------------------" );
				DumpProperties( "Service[rtmp_common]", service_properties, service_settings );


				OBSSource source = obs_source_create( "dshow_input", "Capture", nullptr, nullptr );
				OBSData source_settings = obs_source_get_settings( source );
				obs_properties_t* source_properties = obs_source_properties( source );

				blog( LOG_INFO, "===================================================" );
				blog( LOG_INFO, "Dump 'dshow_input' source:" );
				blog( LOG_INFO, "---------------------------------------------------" );
				DumpProperties( "Source[dshow_input]", source_properties, source_settings );


				OBSEncoder video_encoder = obs_video_encoder_create( "obs_x264", "Video Encoder(x264)", nullptr, nullptr );
				OBSData video_encoder_settings = obs_encoder_get_settings( video_encoder );
				obs_properties_t* video_encoder_properties = obs_encoder_properties( video_encoder );

				blog( LOG_INFO, "===================================================" );
				blog( LOG_INFO, "Dump 'obs_x264' encoder:" );
				blog( LOG_INFO, "---------------------------------------------------" );
				DumpProperties( "Encoder[obs_x264]", video_encoder_properties, video_encoder_settings );


				OBSEncoder audio_encoder = obs_audio_encoder_create( "ffmpeg_aac", "Audio Encoder(AAC)", nullptr, 0,
																	 nullptr );
				OBSData audio_encoder_settings = obs_encoder_get_settings( audio_encoder );
				obs_properties_t* audio_encoder_properties = obs_encoder_properties( audio_encoder );

				blog( LOG_INFO, "===================================================" );
				blog( LOG_INFO, "Dump 'ffmpeg_aac' encoder:" );
				blog( LOG_INFO, "---------------------------------------------------" );
				DumpProperties( "Encoder[ffmpeg_aac]", audio_encoder_properties, audio_encoder_settings );


				OBSOutput streaming_output = obs_output_create( "rtmp_output", "Streaming output", nullptr, nullptr );
				OBSData streaming_output_settings = obs_output_get_settings( streaming_output );
				obs_properties_t* streaming_output_properties = obs_output_properties( streaming_output );

				blog( LOG_INFO, "===================================================" );
				blog( LOG_INFO, "Dump 'rtmp_output' output:" );
				blog( LOG_INFO, "---------------------------------------------------" );
				DumpProperties( "Output[rtmp_output]", streaming_output_properties, streaming_output_settings );

				// configure source & channel
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

				source_settings = obs_source_get_settings( source );
				blog( LOG_INFO, "===================================================" );
				blog( LOG_INFO, "Dump 'dshow_input' source:" );
				blog( LOG_INFO, "---------------------------------------------------" );
				DumpProperties( "Source[dshow_input]", source_properties, source_settings );

				obs_set_output_source( 0, source );
				
				obs_media_state state = obs_source_media_get_state( source );

				// configure video encoder
				video_t* video = obs_get_video();
				enum video_format format = video_output_get_format( video );
				uint32_t w = video_output_get_width( video );
				uint32_t h = video_output_get_height( video );
				double fps = video_output_get_frame_rate( video );

				obs_encoder_set_video( video_encoder, video );
				OBSData ve_settings = obs_data_create();
				obs_data_set_int( ve_settings, "bitrate", 2400 );
				obs_data_set_string( ve_settings, "rate_control", "CBR" );
				obs_data_set_int( ve_settings, "keyint_sec", 1 );
				obs_encoder_update( video_encoder, ve_settings );

				// configure audio encoder
				audio_t* audio = obs_get_audio();
				obs_encoder_set_audio( audio_encoder, audio );
				OBSData ae_settings = obs_data_create();
				obs_data_set_int( ae_settings, "bitrate", 64 );
				obs_encoder_update( audio_encoder, ae_settings );

				// configure service
				OBSData service_settings_new = obs_data_create();
				obs_data_set_string( service_settings_new, "key", "<Twitch key should be here>" );
				obs_data_set_string( service_settings_new, "server", "auto" );
				obs_data_set_string( service_settings_new, "service", "Twitch" );
				obs_service_update( service, service_settings_new );

				// start streaming
				obs_output_set_video_encoder( streaming_output, video_encoder );
				obs_output_set_audio_encoder( streaming_output, audio_encoder, 0 );
				obs_output_set_service( streaming_output, service );
				obs_output_set_delay( streaming_output, 0, 0 );
				obs_output_set_reconnect_settings( streaming_output, 5, 1 );

				bool started = obs_output_start( streaming_output );
				if( started ) {
					blog( LOG_INFO, "Streaming started" );

					printf( "\nPresss any key to stop!\n\n" );
					_getch();

					uint32_t frames_skip = video_output_get_skipped_frames( video );
					uint32_t frames_total = video_output_get_total_frames( video );

					blog( LOG_INFO, "Stats: total_frames=%u, skipped_frames=%u", frames_total, frames_skip );

					obs_output_stop( streaming_output );
				} else {
					blog( LOG_ERROR, "Streaming not started" );
				}

			} else {
				blog( LOG_ERROR, "obs_reset_audio() failed" );
			}

		} else {
			blog( LOG_ERROR, "obs_reset_video() failed, err=%i", err );
		}

		obs_shutdown();
	} else {
		blog( LOG_ERROR, "obs_startup() failed" );
		result = -1;
	}

	return 0;
}
