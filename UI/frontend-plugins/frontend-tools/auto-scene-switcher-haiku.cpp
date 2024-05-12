#include "auto-scene-switcher.hpp"

#include <List.h>
#include <Message.h>
#include <Messenger.h>
#include <Roster.h>
#include <String.h>

void GetWindowList(std::vector<std::string> &windows)
{
	windows.resize(0);

	if (!be_roster)
		return;

	// Let's iterate through all open apps!
	BList appList;
	be_roster->GetAppList(&appList);

	for (int32 appIndex = 0; appIndex < appList.CountItems(); appIndex++) {
		status_t error = B_NO_ERROR;
		BMessenger messenger(NULL, *static_cast<team_id*>(appList.ItemAt(appIndex)), &error);
		if (error != B_OK)
			continue;

		BMessage reply;

		// Get amount of open windows
		BMessage countMessage(B_COUNT_PROPERTIES);
		countMessage.AddSpecifier("Window");

		if (messenger.SendMessage(&countMessage, &reply) != B_OK)
			return;

		int32 windowCount = -1;
		if (reply.FindInt32("result", &windowCount) != B_OK)
			return;

		for (int32 windowIndex = 0; windowIndex < windowCount; windowIndex++) {
			BMessage titleMessage(B_GET_PROPERTY);
			titleMessage.AddSpecifier("Title");
			titleMessage.AddSpecifier("Window", windowIndex);

			if (messenger.SendMessage(&titleMessage, &reply) != B_OK)
				continue;

			BString title;
			if (reply.FindString("result", &title) != B_OK)
				continue;

			windows.emplace_back(title.String());
		}
	}
}

void GetCurrentWindowTitle(std::string &title)
{
	if (!be_roster)
		return;

	app_info info;
	if (be_roster->GetActiveAppInfo(&info) != B_OK)
		return;

	status_t error = B_NO_ERROR;
	BMessenger activeMessenger(NULL, info.team, &error);
	if (error != B_OK)
		return;

	BMessage reply;

	// Retrieve the amount of windows for the active app
	BMessage windowCountMessage(B_COUNT_PROPERTIES);
	windowCountMessage.AddSpecifier("Window");

	if (activeMessenger.SendMessage(&windowCountMessage, &reply) != B_OK)
		return;

	int32 windowCount = 0;
	if (reply.FindInt32("result", &windowCount) != B_OK)
		return;

	// Loop through all windows of the active app to find the active window.
	int32 activeIndex = -1;
	for (int32 index = 0; index < windowCount; index++) {
		BMessage activeCheckMessage(B_GET_PROPERTY);
		activeCheckMessage.AddSpecifier("Active");
		activeCheckMessage.AddSpecifier("Window", index);

		if (activeMessenger.SendMessage(&activeCheckMessage, &reply) != B_OK)
			return;

		bool active = false;
		if (reply.FindBool("result", &active) == B_OK && active) {
			activeIndex = index;
			break;
		}
	}

	if (activeIndex == -1)
		return;

	// Get the title of the active window
	BMessage windowNameMessage(B_GET_PROPERTY);
	windowNameMessage.AddSpecifier("Title");
	windowNameMessage.AddSpecifier("Window", activeIndex);

	if (activeMessenger.SendMessage(&windowNameMessage, &reply) != B_OK)
		return;

	BString activeWindowTitle;
	if (reply.FindString("result", &activeWindowTitle) != B_OK)
		return;

	title.assign(activeWindowTitle.String());
}

void CleanupSceneSwitcher() {}
