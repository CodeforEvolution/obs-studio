/*
Copyright (C) 2024 by Jacob Secunda <secundaja@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <util/platform.h>
#include <util/bmem.h>
#include <util/util_uint64.h>
#include <obs-module.h>

#include <MediaRecorder.h>

#define NSEC_PER_SEC 1000000000LL
#define NSEC_PER_MSEC 1000000L

#define HAIKU_MEDIAKIT_DATA(voidptr) auto *haikuMediaKitData = static_cast<haiku_mediakit_data *>(voidptr);
#define blog(level, msg, ...) blog(level, "haiku-input: " msg, ##__VA_ARGS__)

struct haiku_mediakit_data {
	obs_source_t *source;
//	pa_stream *stream;
	BMediaRecorder *recorder;

	/* user settings */
	char *device;
	bool is_default;
	bool input;

	/* server info */
//	enum speaker_layout speakers;
//	pa_sample_format_t format;
//	uint_fast32_t format;
//	uint_fast32_t samples_per_sec;
//	uint_fast32_t bytes_per_frame;
//	uint_fast8_t channels;
	uint64_t first_ts;

	/* statistics */
	uint_fast32_t packets;
	uint_fast64_t frames;
};

//static void pulse_stop_recording(struct pulse_data *data);

/**
 * get obs from Haiku's raw audio format
 */
static enum audio_format
haiku_to_obs_audio_format(const media_multi_audio_format& format)
{
	switch (format.format) {
	case media_raw_audio_format::B_AUDIO_UCHAR:
		return AUDIO_FORMAT_U8BIT;
	case media_raw_audio_format::B_AUDIO_INT:
		if (format.valid_bits == 16)
			return AUDIO_FORMAT_16BIT;
		else if (format.valid_bits == 32)
			return AUDIO_FORMAT_32BIT;
		else
			return AUDIO_FORMAT_UNKNOWN;
	case media_raw_audio_format::B_AUDIO_FLOAT:
		return AUDIO_FORMAT_FLOAT;
	default:
		return AUDIO_FORMAT_UNKNOWN;
	}
}

/**
 * Get obs speaker layout from number of channels
 *
 * @param channels number of channels reported by pulseaudio
 *
 * @return obs speaker_layout id
 *
 * @note This *might* not work for some rather unusual setups, but should work
 *       fine for the majority of cases.
 */
static enum speaker_layout
haiku_channels_to_obs_speakers(uint_fast32_t channels)
{
	switch (channels) {
	case 1:
		return SPEAKERS_MONO;
	case 2:
		return SPEAKERS_STEREO;
	case 3:
		return SPEAKERS_2POINT1;
	case 4:
		return SPEAKERS_4POINT0;
	case 5:
		return SPEAKERS_4POINT1;
	case 6:
		return SPEAKERS_5POINT1;
	case 8:
		return SPEAKERS_7POINT1;
	}

	return SPEAKERS_UNKNOWN;
}

static uint_fast32_t haiku_channel_map(enum speaker_layout layout)
{
	switch (layout) {
	case SPEAKERS_MONO:
		return B_CHANNEL_CENTER;

	case SPEAKERS_STEREO:
		return B_CHANNEL_LEFT | B_CHANNEL_RIGHT;

	case SPEAKERS_2POINT1:
		return B_CHANNEL_LEFT | B_CHANNEL_RIGHT | B_CHANNEL_SUB;

	case SPEAKERS_4POINT0:
		return B_CHANNEL_LEFT | B_CHANNEL_CENTER | B_CHANNEL_RIGHT | B_CHANNEL_BACK_CENTER;

	case SPEAKERS_4POINT1:
		return B_CHANNEL_LEFT | B_CHANNEL_CENTER | B_CHANNEL_RIGHT | B_CHANNEL_BACK_CENTER | B_CHANNEL_SUB;

	case SPEAKERS_5POINT1:
		return B_CHANNEL_LEFT | B_CHANNEL_CENTER | B_CHANNEL_RIGHT | B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT | B_CHANNEL_SUB;

	case SPEAKERS_7POINT1:
		return B_CHANNEL_LEFT | B_CHANNEL_CENTER | B_CHANNEL_RIGHT | B_CHANNEL_SIDE_LEFT | B_CHANNEL_SIDE_RIGHT | B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT | B_CHANNEL_SUB;

	case SPEAKERS_UNKNOWN:
	default:
		return 0;
	}
}

static inline uint64_t samples_to_ns(size_t frames, uint_fast32_t rate)
{
	return util_mul_div64(frames, NSEC_PER_SEC, rate);
}

static inline uint64_t get_sample_time(size_t frames, uint_fast32_t rate)
{
	return os_gettime_ns() - samples_to_ns(frames, rate);
}

#define STARTUP_TIMEOUT_NS (500 * NSEC_PER_MSEC)

/**
 * Callback for haiku which gets executed when new audio data is available
 */
static void haiku_process_audio(void *cookie, bigtime_t timestamp, void *data, size_t size, const media_format& format)
{
//	UNUSED_PARAMETER(timestamp);

	HAIKU_MEDIAKIT_DATA(cookie);
	if (!haikuMediaKitData || !haikuMediaKitData->recorder)
		return;

	// check if we got data
	if (!size)
		return;

	if (!data) {
		blog(LOG_ERROR, "Got audio hole of %zu bytes", size);
		return;
	}

	if (!format.IsAudio()) {
		blog(LOG_ERROR, "Didn't receive audio data?!?");
		return;
	}

	struct obs_source_audio out;
	out.speakers = haiku_channels_to_obs_speakers(format.u.raw_audio.channels);
	out.samples_per_sec = format.u.raw_audio.frame_rate;
	out.format = haiku_to_obs_audio_format(format.u.raw_audio);
	out.data[0] = static_cast<uint8_t *>(data);
	out.frames = size / format.AudioFrameSize();
//	out.timestamp = get_sample_time(out.frames, out.samples_per_sec);
	out.timestamp = timestamp;

	if (!data->first_ts)
		data->first_ts = out.timestamp + STARTUP_TIMEOUT_NS;

	if (out.timestamp > data->first_ts)
		obs_source_output_audio(data->source, &out);

	data->packets++;
	data->frames += out.frames;
}

/**
 * Server info callback
 */
static void pulse_server_info(pa_context *c, const pa_server_info *i,
			      void *userdata)
{
	UNUSED_PARAMETER(c);
	PULSE_DATA(userdata);

	blog(LOG_INFO, "Server name: '%s %s'", i->server_name,
	     i->server_version);

	if (data->is_default) {
		bfree(data->device);
		if (data->input) {
			data->device = bstrdup(i->default_source_name);

			blog(LOG_DEBUG, "Default input device: '%s'",
			     data->device);
		} else {
			char *monitor =
				bzalloc(strlen(i->default_sink_name) + 9);
			strcat(monitor, i->default_sink_name);
			strcat(monitor, ".monitor");

			data->device = bstrdup(monitor);

			blog(LOG_DEBUG, "Default output device: '%s'",
			     data->device);
			bfree(monitor);
		}
	}

	pulse_signal(0);
}

/**
 * Source info callback
 *
 * We use the default stream settings for recording here unless pulse is
 * configured to something obs can't deal with.
 */
static void pulse_source_info(pa_context *c, const pa_source_info *i, int eol,
			      void *userdata)
{
	UNUSED_PARAMETER(c);
	PULSE_DATA(userdata);
	// An error occured
	if (eol < 0) {
		data->format = PA_SAMPLE_INVALID;
		goto skip;
	}
	// Terminating call for multi instance callbacks
	if (eol > 0)
		goto skip;

	blog(LOG_INFO,
	     "Audio format: %s, %" PRIu32 " Hz"
	     ", %" PRIu8 " channels",
	     pa_sample_format_to_string(i->sample_spec.format),
	     i->sample_spec.rate, i->sample_spec.channels);

	pa_sample_format_t format = i->sample_spec.format;
	if (pulse_to_obs_audio_format(format) == AUDIO_FORMAT_UNKNOWN) {
		format = PA_SAMPLE_FLOAT32LE;

		blog(LOG_INFO,
		     "Sample format %s not supported by OBS,"
		     "using %s instead for recording",
		     pa_sample_format_to_string(i->sample_spec.format),
		     pa_sample_format_to_string(format));
	}

	uint8_t channels = i->sample_spec.channels;
	if (pulse_channels_to_obs_speakers(channels) == SPEAKERS_UNKNOWN) {
		channels = 2;

		blog(LOG_INFO,
		     "%c channels not supported by OBS,"
		     "using %c instead for recording",
		     i->sample_spec.channels, channels);
	}

	data->format = format;
	data->samples_per_sec = i->sample_spec.rate;
	data->channels = channels;

skip:
	pulse_signal(0);
}

/**
 * Start recording
 *
 * We request the default format used by Haiku's MediaKit here because the data will be
 * converted and possibly re-sampled by obs anyway.
 */
static int_fast32_t haiku_start_recording(struct haiku_mediakit_data *data)
{
	if (pulse_get_server_info(pulse_server_info, (void *)data) < 0) {
		blog(LOG_ERROR, "Unable to get server info !");
		return -1;
	}

	if (pulse_get_source_info(pulse_source_info, data->device,
				  (void *)data) < 0) {
		blog(LOG_ERROR, "Unable to get source info !");
		return -1;
	}
	if (data->format == PA_SAMPLE_INVALID) {
		blog(LOG_ERROR,
		     "An error occurred while getting the source info!");
		return -1;
	}

	pa_sample_spec spec;
	spec.format = data->format;
	spec.rate = data->samples_per_sec;
	spec.channels = data->channels;

	if (!pa_sample_spec_valid(&spec)) {
		blog(LOG_ERROR, "Sample spec is not valid");
		return -1;
	}

	data->speakers = pulse_channels_to_obs_speakers(spec.channels);
	data->bytes_per_frame = pa_frame_size(&spec);

	pa_channel_map channel_map = pulse_channel_map(data->speakers);

	data->stream = pulse_stream_new(obs_source_get_name(data->source),
					&spec, &channel_map);
	if (!data->stream) {
		blog(LOG_ERROR, "Unable to create stream");
		return -1;
	}

	pulse_lock();
	pa_stream_set_read_callback(data->stream, pulse_stream_read,
				    (void *)data);
	pulse_unlock();

	pa_buffer_attr attr;
	attr.fragsize = pa_usec_to_bytes(25000, &spec);
	attr.maxlength = (uint32_t)-1;
	attr.minreq = (uint32_t)-1;
	attr.prebuf = (uint32_t)-1;
	attr.tlength = (uint32_t)-1;

	pa_stream_flags_t flags = PA_STREAM_ADJUST_LATENCY;
	if (!data->is_default)
		flags |= PA_STREAM_DONT_MOVE;

	pulse_lock();
	int_fast32_t ret = pa_stream_connect_record(data->stream, data->device,
						    &attr, flags);
	pulse_unlock();
	if (ret < 0) {
		pulse_stop_recording(data);
		blog(LOG_ERROR, "Unable to connect to stream");
		return -1;
	}

	if (data->is_default)
		blog(LOG_INFO, "Started recording from '%s' (default)",
		     data->device);
	else
		blog(LOG_INFO, "Started recording from '%s'", data->device);

	return 0;
}

/**
 * stop recording
 */
static void haiku_stop_recording(struct pulse_data *data)
{
	if (data->stream) {
		pulse_lock();
		pa_stream_disconnect(data->stream);
		pa_stream_unref(data->stream);
		data->stream = NULL;
		pulse_unlock();
	}

	blog(LOG_INFO, "Stopped recording from '%s'", data->device);
	blog(LOG_INFO,
	     "Got %" PRIuFAST32 " packets with %" PRIuFAST64 " frames",
	     data->packets, data->frames);

	data->first_ts = 0;
	data->packets = 0;
	data->frames = 0;
}

/**
 * input info callback
 */
static void pulse_input_info(pa_context *c, const pa_source_info *i, int eol,
			     void *userdata)
{
	UNUSED_PARAMETER(c);
	if (eol != 0 || i->monitor_of_sink != PA_INVALID_INDEX)
		goto skip;

	obs_property_list_add_string((obs_property_t *)userdata, i->description,
				     i->name);

skip:
	pulse_signal(0);
}

/**
 * output info callback
 */
static void pulse_output_info(pa_context *c, const pa_sink_info *i, int eol,
			      void *userdata)
{
	UNUSED_PARAMETER(c);
	if (eol != 0 || i->monitor_source == PA_INVALID_INDEX)
		goto skip;

	obs_property_list_add_string((obs_property_t *)userdata, i->description,
				     i->monitor_source_name);

skip:
	pulse_signal(0);
}

/**
 * Get plugin properties
 */
static obs_properties_t *haiku_properties(bool input)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *devices = obs_properties_add_list(
		props, "device_id", obs_module_text("Device"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

//	pulse_init();
	if (input)
		pulse_get_source_info_list(pulse_input_info, (void *)devices);
	else
		pulse_get_sink_info_list(pulse_output_info, (void *)devices);
//	pulse_unref();

	size_t count = obs_property_list_item_count(devices);

	if (count > 0)
		obs_property_list_insert_string(
			devices, 0, obs_module_text("Default"), "default");

	return props;
}

static obs_properties_t *haiku_input_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	return pulse_properties(true);
}

static obs_properties_t *haiku_output_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	return pulse_properties(false);
}

/**
 * Get plugin defaults
 */
static void haiku_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "device_id", "default");
}

/**
 * Returns the name of the plugin
 */
static const char *haiku_input_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("HaikuMediaKitInput");
}

static const char *haiku_output_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("HaikuMediaKitOutput");
}

/**
 * Destroy the plugin object and free all memory
 */
static void haiku_destroy(void *vptr)
{
	HAIKU_MEDIAKIT_DATA(vptr);

	if (!data)
		return;

	if (data->stream)
		pulse_stop_recording(data);
	pulse_unref();

	if (data->device)
		bfree(data->device);
	bfree(data);
}

/**
 * Update the input settings
 */
static void haiku_update(void *vptr, obs_data_t *settings)
{
	HAIKU_MEDIAKIT_DATA(vptr);
	bool restart = false;
	const char *new_device;

	new_device = obs_data_get_string(settings, "device_id");
	if (!data->device || strcmp(data->device, new_device) != 0) {
		if (data->device)
			bfree(data->device);
		data->device = bstrdup(new_device);
		data->is_default = strcmp("default", data->device) == 0;
		restart = true;
	}

	if (!restart)
		return;

	if (data->stream)
		pulse_stop_recording(data);
	pulse_start_recording(data);
}

/**
 * Create the plugin object
 */
static void *haiku_create(obs_data_t *settings, obs_source_t *source,
			  bool input)
{
	struct haiku_mediakit_data *data = static_cast<haiku_mediakit_data *>(bzalloc(sizeof(struct haiku_mediakit_data)));
	if (!data)
		return nullptr;

	data->input = input;
	data->source = source;

//	pulse_init();
	haiku_update(data, settings);

	return data;
}

static void *haiku_input_create(obs_data_t *settings, obs_source_t *source)
{
	return haiku_create(settings, source, true);
}

static void *haiku_output_create(obs_data_t *settings, obs_source_t *source)
{
	return haiku_create(settings, source, false);
}

struct obs_source_info haiku_input_capture = {
	.id = "haiku_input_capture",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE,
	.get_name = haiku_input_getname,
	.create = haiku_input_create,
	.destroy = haiku_destroy,
	.update = haiku_update,
	.get_defaults = haiku_defaults,
	.get_properties = haiku_input_properties,
	.icon_type = OBS_ICON_TYPE_AUDIO_INPUT,
};

struct obs_source_info haiku_output_capture = {
	.id = "haiku_output_capture",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE |
			OBS_SOURCE_DO_NOT_SELF_MONITOR,
	.get_name = haiku_output_getname,
	.create = haiku_output_create,
	.destroy = haiku_destroy,
	.update = haiku_update,
	.get_defaults = haiku_defaults,
	.get_properties = haiku_output_properties,
	.icon_type = OBS_ICON_TYPE_AUDIO_OUTPUT,
};
