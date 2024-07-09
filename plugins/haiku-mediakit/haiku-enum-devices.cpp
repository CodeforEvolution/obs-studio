#include "../../obs-internal.h"
#include "../../util/dstr.h"

#include <MediaRoster.h>

static bool obs_enum_audio_monitoring_device(obs_enum_audio_device_cb cb, void *data, live_node_info &info)
{
	bool cont = false;
	char *name = nullptr;
	char *uid = nullptr;

	name = bstrdup(info.name);

	int uidLen = snprintf(nullptr, 0, "%i", static_cast<int32>(info.node.node));
	uid = static_cast<char *>(bmalloc(uidLen + 1));
	if (uid == nullptr)
		goto fail;

	snprintf(uid, uidLen + 1, "%i", static_cast<int32>(info.node.node));

	cont = cb(data, name, uid);

fail:
	bfree(name);
	bfree(uid);

	return cont;
}

static void enum_audio_devices(obs_enum_audio_device_cb cb, void *data)
{
	status_t error = B_NO_ERROR;
	auto roster = BMediaRoster::Roster(&error);
	if (roster == nullptr || error != B_OK)
		return;

	#define MAX_LIVE 64
	live_node_info liveInfos[MAX_LIVE];
	int32 liveCount = MAX_LIVE;
	error = roster->GetLiveNodes(liveInfos, &liveCount, NULL, NULL, NULL, B_BUFFER_CONSUMER | B_PHYSICAL_OUTPUT);
	if (error != B_OK)
		return;

	for (int32 liveIndex = 0; liveIndex < liveCount; liveIndex++) {
		if (!obs_enum_audio_monitoring_device(cb, data, liveInfos[liveIndex]))
			break;
	}
}

void obs_enum_audio_monitoring_devices(obs_enum_audio_device_cb cb, void *data)
{
	enum_audio_devices(cb, data);
}

static bool alloc_default_id(void *data, const char *name, const char *id)
{
	char **p_id = static_cast<char **>(data);
	UNUSED_PARAMETER(name);

	*p_id = bstrdup(id);
	return false;
}

static void get_default_id(char **p_id)
{
	if (*p_id)
		return;

	status_t error = B_NO_ERROR;
	media_node output;
	live_node_info info;

	auto roster = BMediaRoster::Roster(&error);
	if (roster == nullptr || error != B_OK)
		goto error;


	error = roster->GetAudioOutput(&output);
	if (error != B_OK)
		goto error;

	error = roster->GetLiveNodeInfo(output, &info);
	if (error != B_OK) {
		roster->ReleaseNode(output);
		goto error;
	}


	obs_enum_audio_monitoring_device(alloc_default_id, p_id, info);

error:
	if (!*p_id)
		*p_id = static_cast<char *>(bzalloc(1));
}

struct device_name_info {
	const char *id;
	char *name;
};

static bool enum_device_name(void *data, const char *name, const char *id)
{
	struct device_name_info *info = static_cast<struct device_name_info *>(data);

	if (strcmp(info->id, id) == 0) {
		info->name = bstrdup(name);
		return false;
	}

	return true;
}

bool devices_match(const char *id1, const char *id2)
{
	struct device_name_info info = {0};
	char *default_id = NULL;
	char *name1 = NULL;
	char *name2 = NULL;
	bool match;

	if (!id1 || !id2)
		return false;

	if (strcmp(id1, "0") == 0) {
		get_default_id(&default_id);
		id1 = default_id;
	}
	if (strcmp(id2, "0") == 0) {
		get_default_id(&default_id);
		id2 = default_id;
	}

	info.id = id1;
	enum_audio_devices(enum_device_name, &info);
	name1 = info.name;

	info.name = NULL;
	info.id = id2;
	enum_audio_devices(enum_device_name, &info);
	name2 = info.name;

	match = name1 && name2 && strcmp(name1, name2) == 0;
	bfree(default_id);
	bfree(name1);
	bfree(name2);

	return match;
}
