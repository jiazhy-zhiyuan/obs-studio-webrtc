#include <stdio.h>
#include <obs-module.h>
#include <obs-avc.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <util/threading.h>
#include <inttypes.h>
#include <rtc_base/platform_file.h>
#include <rtc_base/bitrateallocationstrategy.h>
#include <modules/audio_processing/include/audio_processing.h>

#define warn(format, ...)  blog(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  blog(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) blog(LOG_DEBUG,   format, ##__VA_ARGS__)

#define OPT_DROP_THRESHOLD "drop_threshold_ms"
#define OPT_PFRAME_DROP_THRESHOLD "pframe_drop_threshold_ms"
#define OPT_MAX_SHUTDOWN_TIME_SEC "max_shutdown_time_sec"
#define OPT_BIND_IP "bind_ip"
#define OPT_NEWSOCKETLOOP_ENABLED "new_socket_loop_enabled"
#define OPT_LOWLATENCY_ENABLED "low_latency_mode_enabled"

#include "WebRTCStream.h"

extern "C" const char *sportsbooth_stream_getname(void *unused)
{
	info("sportsbooth_stream_getname");
	UNUSED_PARAMETER(unused);
	return obs_module_text("SPORTSBOOTHStream");
}

extern "C" void sportsbooth_stream_destroy(void *data)
{
	info("sportsbooth_stream_destroy");
	//Get stream
	WebRTCStream* stream = (WebRTCStream*)data;
	//Stop it
	stream->stop();
	//Remove ref and let it self destroy
	stream->Release();
}

extern "C" void *sportsbooth_stream_create(obs_data_t *settings, obs_output_t *output)
{
	info("sportsbooth_stream_create");
	//Create new stream
	WebRTCStream* stream = new WebRTCStream(output);
	//Don't allow it to be deleted
	stream->AddRef();
	//Return it
	return (void*)stream;
}

extern "C" void sportsbooth_stream_stop(void *data, uint64_t ts)
{
	info("sportsbooth_stream_stop");
	//Get stream
	WebRTCStream* stream = (WebRTCStream*)data;
	//Stop it
	stream->stop();
	//Remove ref and let it self destroy
	stream->Release();
}

extern "C" bool sportsbooth_stream_start(void *data)
{
	info("sportsbooth_stream_start");
	//Get stream
	WebRTCStream* stream = (WebRTCStream*)data;
	//Don't allow it to be deleted
	stream->AddRef();
	//Start it
	return stream->start(WebRTCStream::SportsBooth);
}

extern "C" void sportsbooth_receive_video(void *data, struct video_data *frame)
{
	//Get stream
	WebRTCStream* stream = (WebRTCStream*)data;
	//Process audio
	stream->onVideoFrame(frame);
}
extern "C" void sportsbooth_receive_audio(void *data, struct audio_data *frame)
{
	//Get stream
	WebRTCStream* stream = (WebRTCStream*)data;
	//Process audio
	stream->onAudioFrame(frame);
}

extern "C" void sportsbooth_stream_defaults(obs_data_t *defaults)
{
	info("sportsbooth_stream_defaults");
	obs_data_set_default_int(defaults, OPT_DROP_THRESHOLD, 700);
	obs_data_set_default_int(defaults, OPT_PFRAME_DROP_THRESHOLD, 900);
	obs_data_set_default_int(defaults, OPT_MAX_SHUTDOWN_TIME_SEC, 30);
	obs_data_set_default_string(defaults, OPT_BIND_IP, "default");
	obs_data_set_default_bool(defaults, OPT_NEWSOCKETLOOP_ENABLED, false);
	obs_data_set_default_bool(defaults, OPT_LOWLATENCY_ENABLED, false);
}

extern "C" obs_properties_t *sportsbooth_stream_properties(void *unused)
{
	info("sportsbooth_stream_properties");
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_int(props, OPT_DROP_THRESHOLD,
			obs_module_text("SPORTSBOOTHStream.DropThreshold"),
			200, 10000, 100);

	obs_properties_add_bool(props, OPT_NEWSOCKETLOOP_ENABLED,
			obs_module_text("SPORTSBOOTHStream.NewSocketLoop"));
	obs_properties_add_bool(props, OPT_LOWLATENCY_ENABLED,
			obs_module_text("SPORTSBOOTHStream.LowLatencyMode"));

	return props;
}

extern "C" uint64_t sportsbooth_stream_total_bytes_sent(void *data)
{
	//Get stream
	WebRTCStream* stream = (WebRTCStream*)data;
	return stream->getBitrate();
}

extern "C" int sportsbooth_stream_dropped_frames(void *data)
{
  //Get stream
  WebRTCStream* stream = (WebRTCStream*)data;
  return stream->getDroppedFrame();
}

extern "C" float sportsbooth_stream_congestion(void *data)
{
	return 0.0f;
}

extern "C" {
	struct obs_output_info sportsbooth_output_info = {
		"sportsbooth_output", //id
		OBS_OUTPUT_AV |	OBS_OUTPUT_SERVICE | OBS_OUTPUT_MULTI_TRACK, //flags
		sportsbooth_stream_getname, //get_name
		sportsbooth_stream_create, //create
		sportsbooth_stream_destroy, //destroy
		sportsbooth_stream_start, //start
		sportsbooth_stream_stop, //stop
		sportsbooth_receive_video, //raw_video
		sportsbooth_receive_audio, //raw_audio
		nullptr, //encoded_packet
		nullptr, //update
		sportsbooth_stream_defaults, //get_defaults
		sportsbooth_stream_properties, //get_properties
		nullptr, //pause
		sportsbooth_stream_total_bytes_sent, //get_total_bytes
		sportsbooth_stream_dropped_frames, //get_dropped_frame
		nullptr, //type_data
		nullptr, ////free_type_data
		sportsbooth_stream_congestion, //get_congestion
		nullptr, //get_connect_time_ms
		"vp8", //encoded_video_codecs
		"opus" //encoded_audio_codecs
	};
}
