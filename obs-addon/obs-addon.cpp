#include <node.h>
#include "streamer.h"

using namespace v8;

class AddonData {
	twitch_streamer::CStreamer::Ptr m_Object;

public:
	AddonData( Isolate* isolate, const twitch_streamer::CStreamer::Ptr& obj ) : m_Object( obj ) {
		assert( obj != nullptr );
		node::AddEnvironmentCleanupHook( isolate, DeleteInstance, this );
	}

	twitch_streamer::CStreamer::Ptr Obj() {
		return m_Object;
	}

private:
	static void DeleteInstance( void* data ) {
		auto addon_data = static_cast<AddonData*>( data );
		assert( addon_data != nullptr );
		addon_data->Obj()->Shutdown();
		delete addon_data;
	}
};

static void GetVersion( const v8::FunctionCallbackInfo<v8::Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	args.GetReturnValue().Set(
		String::NewFromUtf8( isolate, twitch_streamer::CStreamer::GetVersion().c_str() ).ToLocalChecked() );
}

static void GetOBSVersion( const v8::FunctionCallbackInfo<v8::Value>& args ) {
	Isolate* isolate = args.GetIsolate();

	args.GetReturnValue().Set(
		String::NewFromUtf8( isolate, twitch_streamer::CStreamer::GetOBSVersion().c_str() ).ToLocalChecked() );
}

static void IsStarted( const v8::FunctionCallbackInfo<v8::Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	AddonData* data = reinterpret_cast<AddonData*>( args.Data().As<External>()->Value() );

	args.GetReturnValue().Set( data->Obj()->IsStarted() );
}

static void Start( const v8::FunctionCallbackInfo<v8::Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	AddonData* data = reinterpret_cast<AddonData*>( args.Data().As<External>()->Value() );

	if( args.Length() != 1 ) {
		isolate->ThrowException( Exception::TypeError(
			String::NewFromUtf8( isolate, "Wrong number of arguments. Expected one" ).ToLocalChecked() ) );
		return;
	}

	v8::String::Utf8Value key( args.GetIsolate(), args[ 0 ] );

	args.GetReturnValue().Set( data->Obj()->Start( *key ) );
}

static void Stop( const v8::FunctionCallbackInfo<v8::Value>& args ) {
	Isolate* isolate = args.GetIsolate();
	AddonData* data = reinterpret_cast<AddonData*>( args.Data().As<External>()->Value() );

	data->Obj()->Stop();
}

// Initialize this addon to be context-aware.
NODE_MODULE_INIT( /* exports, module, context */ ) {
	Isolate* isolate = context->GetIsolate();

	// Create a new instance of `AddonData` for this instance of the addon and
	// tie its life cycle to that of the Node.js environment.
	auto streamer = std::make_shared<twitch_streamer::CStreamer>();
	if( streamer->Init() != 0 ) {
		isolate->ThrowException(
			Exception::TypeError( String::NewFromUtf8( isolate, "Initialization failed" ).ToLocalChecked() ) );
		return;
	}
	AddonData* data = new AddonData( isolate, streamer );

	// Wrap the data in a `v8::External` so we can pass it to the method we
	// expose.
	Local<External> external = External::New( isolate, data );

	// Expose the method `Method` to JavaScript, and make sure it receives the
	// per-addon-instance data we created above by passing `external` as the
	// third parameter to the `FunctionTemplate` constructor.
	exports
		->Set( context, String::NewFromUtf8( isolate, "GetVersion" ).ToLocalChecked(),
			   FunctionTemplate::New( isolate, GetVersion, external )->GetFunction( context ).ToLocalChecked() )
		.FromJust();

	exports
		->Set( context, String::NewFromUtf8( isolate, "GetOBSVersion" ).ToLocalChecked(),
			   FunctionTemplate::New( isolate, GetOBSVersion, external )->GetFunction( context ).ToLocalChecked() )
		.FromJust();

	exports
		->Set( context, String::NewFromUtf8( isolate, "IsStarted" ).ToLocalChecked(),
			   FunctionTemplate::New( isolate, IsStarted, external )->GetFunction( context ).ToLocalChecked() )
		.FromJust();

	exports
		->Set( context, String::NewFromUtf8( isolate, "Start" ).ToLocalChecked(),
			   FunctionTemplate::New( isolate, Start, external )->GetFunction( context ).ToLocalChecked() )
		.FromJust();

	exports
		->Set( context, String::NewFromUtf8( isolate, "Stop" ).ToLocalChecked(),
			   FunctionTemplate::New( isolate, Stop, external )->GetFunction( context ).ToLocalChecked() )
		.FromJust();
}
