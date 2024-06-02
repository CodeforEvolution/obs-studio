#include <obs-internal.h>

#include <deque>

#include <Buffer.h>
#include <BufferGroup.h>
#include <MediaDefs.h>
#include <MediaRoster.h>
#include <SoundPlayer.h>

struct audio_monitor {
	obs_source_t *source;
	BSoundPlayer *player;
	BBufferGroup *buffer_group;

	pthread_mutex_t mutex;

	std::deque<BBuffer*>* filled_buffers;
	audio_resampler_t *resampler;
	size_t buffer_size;
	uint32_t channels;

	volatile bool active;
	bool ignore;
};

static enum speaker_layout
haiku_channels_to_obs_speakers(uint_fast32_t channels)
{
	switch (channels) {
	case 0:
		return SPEAKERS_UNKNOWN;
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
	default:
		return SPEAKERS_UNKNOWN;
	}
}

static enum audio_format
haiku_to_obs_audio_format(media_multi_audio_format format)
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

static uint32 haiku_channel_map(enum speaker_layout layout)
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

static void on_audio_pause(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);
	struct audio_monitor *monitor = static_cast<audio_monitor *>(data);
	pthread_mutex_lock(&monitor->mutex);
	monitor->player->SetHasData(false);
	monitor->filled_buffers->clear();
	pthread_mutex_unlock(&monitor->mutex);
}

static void on_audio_playback(void *param, obs_source_t *source,
			      const struct audio_data *audio_data, bool muted)
{
	struct audio_monitor *monitor = static_cast<audio_monitor *>(param);
	float vol = source->user_volume;
	uint32_t bytes;

	if (!os_atomic_load_bool(&monitor->active)) {
		return;
	}

	if (os_atomic_load_long(&source->activate_refs) == 0) {
		return;
	}

	uint8_t *resample_data[MAX_AV_PLANES];
	uint32_t resample_frames;
	uint64_t ts_offset;
	bool success;

	success = audio_resampler_resample(
		monitor->resampler, resample_data, &resample_frames, &ts_offset,
		reinterpret_cast<const uint8_t *const *>(audio_data->data),
		static_cast<uint32_t>(audio_data->frames));
	if (!success) {
		return;
	}

	bytes = sizeof(float) * monitor->channels * resample_frames;

	if (muted) {
		memset(resample_data[0], 0, bytes);
	} else {
		/* apply volume */
		if (!close_float(vol, 1.0f, EPSILON)) {
			float *cur = reinterpret_cast<float *>(resample_data[0]);
			float *end = cur + resample_frames * monitor->channels;

			while (cur < end)
				*(cur++) *= vol;
		}
	}

	pthread_mutex_lock(&monitor->mutex);

	BBuffer *buffer = nullptr;
	status_t result = monitor->buffer_group->RequestBuffer(buffer, -1);
	if (result != B_OK)
		return;

	memcpy(buffer->Data(), resample_data[0], bytes);
	buffer->SetSizeUsed(bytes);

	monitor->filled_buffers->push_back(buffer);
	monitor->player->SetHasData(true);

	pthread_mutex_unlock(&monitor->mutex);
}

static void buffer_audio(void *cookie, void *output_buffer, size_t size, const media_raw_audio_format& format)
{
	UNUSED_PARAMETER(format);

	auto monitor = static_cast<struct audio_monitor*>(cookie);

	pthread_mutex_lock(&monitor->mutex);
	if (!monitor->filled_buffers->empty()) {
		monitor->filled_buffers->pop_front();
		BBuffer *buffer = monitor->filled_buffers->front();

		memcpy(output_buffer, buffer->Data(), size);
		buffer->Recycle();
	} else {
		// Fill with silence...
		memset(output_buffer, 0, size);
	}

	if (monitor->filled_buffers->empty())
		monitor->player->SetHasData(false);

	pthread_mutex_unlock(&monitor->mutex);
}

extern bool devices_match(const char *id1, const char *id2);

static bool audio_monitor_init(struct audio_monitor *monitor,
			       obs_source_t *source)
{
	const struct audio_output_info *info =
		audio_output_get_info(obs->audio.audio);

	status_t error = B_NO_ERROR;

	auto roster = BMediaRoster::Roster(&error);
	if (error != B_OK) {
		// TODO: Log error!
		return false;
	}

	media_multi_audio_format format = media_multi_audio_format::wildcard;
	format.frame_rate = static_cast<float>(info->samples_per_sec);
	format.channel_count = get_audio_channels(info->speakers);
	format.format = media_raw_audio_format::B_AUDIO_FLOAT;
	format.byte_order = B_LITTLE_ENDIAN;
	format.buffer_size = roster->AudioBufferSizeFor(format.channel_count, format.format, format.frame_rate);
	format.channel_mask = haiku_channel_map(info->speakers);

	monitor->source = source;

	monitor->channels = format.channel_count;
	monitor->buffer_size =
		format.channel_count * sizeof(float) * info->samples_per_sec / 100 * 3;

	pthread_mutex_init_value(&monitor->mutex);

	const char *uid = obs->audio.monitoring_device_id;
	if (!uid || !*uid) {
		return false;
	}

	if (source->info.output_flags & OBS_SOURCE_DO_NOT_SELF_MONITOR) {
		obs_data_t *s = obs_source_get_settings(source);
		const char *s_dev_id = obs_data_get_string(s, "device_id");
		bool match = devices_match(s_dev_id, uid);
		obs_data_release(s);

		if (match) {
			monitor->ignore = true;
			return true;
		}
	}

	media_node input_node;

	// What's our selected device?
	if (strcmp(uid, "default") != 0) {
		// Convert UID string back to media_node_id
		auto node_id = static_cast<media_node_id>(strtol(uid, nullptr, 10));
		error = roster->GetNodeFor(node_id, &input_node);
		if (error != B_OK) {
			// TODO: Log error!
			return false;
		}
	} else {
		// Default time!
		error = roster->GetAudioOutput(&input_node);
		if (error != B_OK) {
			// TODO: Log error!
			return false;
		}
	}

	monitor->player = new(std::nothrow) BSoundPlayer(input_node, &format, "OBS Studio Audio Monitor", nullptr, buffer_audio, nullptr, monitor);
	if (monitor->player == nullptr || monitor->player->InitCheck() != B_OK) {
		// TODO: Log error!
		roster->ReleaseNode(input_node);
		return false;
	}
	monitor->player->SetVolume(1.0);

	monitor->buffer_group = new(std::nothrow) BBufferGroup(monitor->buffer_size);
	if (monitor->buffer_group == nullptr || monitor->buffer_group->InitCheck() != B_OK) {
		// TODO: Log error!
		return false;
	}

	if (pthread_mutex_init(&monitor->mutex, NULL) != 0) {
		blog(LOG_WARNING, "%s: %s", __FUNCTION__,
		     "Failed to init mutex");
		return false;
	}

	monitor->filled_buffers = new(std::nothrow) std::deque<BBuffer*>();
	if (monitor->filled_buffers == nullptr) {
		blog(LOG_WARNING, "%s: %s", __FUNCTION__,
		     "Failed to init filled buffer deque");
		return false;
	}

	struct resample_info from = {
		.samples_per_sec = info->samples_per_sec,
		.format = AUDIO_FORMAT_FLOAT_PLANAR,
		.speakers = info->speakers
	};

	struct resample_info to = {
		.samples_per_sec = info->samples_per_sec,
		.format = AUDIO_FORMAT_FLOAT,
		.speakers = info->speakers
	};

	monitor->resampler = audio_resampler_create(&to, &from);
	if (!monitor->resampler) {
		blog(LOG_WARNING, "%s: %s", __FUNCTION__,
		     "Failed to create resampler");
		return false;
	}

	error = monitor->player->Start();
	if (error != B_OK) {
		// TODO: Log error!
		return false;
	}

	monitor->active = true;

	return true;
}

static void audio_monitor_free(struct audio_monitor *monitor)
{
	if (monitor->source) {
		obs_source_remove_audio_capture_callback(
			monitor->source, on_audio_playback, monitor);
		obs_source_remove_audio_pause_callback(monitor->source,
						       on_audio_pause, monitor);
	}

	if (monitor->active) {
		monitor->player->Stop();
	}
	delete monitor->player;

	monitor->filled_buffers->clear();
	delete monitor->filled_buffers;

	delete monitor->buffer_group;

	audio_resampler_destroy(monitor->resampler);
	pthread_mutex_destroy(&monitor->mutex);
}

static void audio_monitor_init_final(struct audio_monitor *monitor)
{
	if (monitor->ignore)
		return;

	obs_source_add_audio_capture_callback(monitor->source,
					      on_audio_playback, monitor);
	obs_source_add_audio_pause_callback(monitor->source, on_audio_pause,
					    monitor);
}

struct audio_monitor *audio_monitor_create(obs_source_t *source)
{
	struct audio_monitor *monitor = static_cast<struct audio_monitor *>(bzalloc(sizeof(*monitor)));

	if (!audio_monitor_init(monitor, source)) {
		goto fail;
	}

	pthread_mutex_lock(&obs->audio.monitoring_mutex);
	da_push_back(obs->audio.monitors, &monitor);
	pthread_mutex_unlock(&obs->audio.monitoring_mutex);

	audio_monitor_init_final(monitor);
	return monitor;

fail:
	audio_monitor_free(monitor);
	bfree(monitor);
	return NULL;
}

void audio_monitor_reset(struct audio_monitor *monitor)
{
	bool success;

	obs_source_t *source = monitor->source;
	audio_monitor_free(monitor);
	memset(monitor, 0, sizeof(*monitor));

	success = audio_monitor_init(monitor, source);
	if (success)
		audio_monitor_init_final(monitor);
}

void audio_monitor_destroy(struct audio_monitor *monitor)
{
	if (monitor) {
		audio_monitor_free(monitor);

		pthread_mutex_lock(&obs->audio.monitoring_mutex);
		da_erase_item(obs->audio.monitors, &monitor);
		pthread_mutex_unlock(&obs->audio.monitoring_mutex);

		bfree(monitor);
	}
}
