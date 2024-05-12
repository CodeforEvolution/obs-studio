/******************************************************************************
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
******************************************************************************/

#include <obs-config.h>

#include "obs-app.hpp"
#include "platform.hpp"

#include <sstream>

#include <FindDirectory.h>
#include <LocaleRoster.h>
#include <Path.h>
#include <String.h>
#include <StringList.h>

#define INSTALL_DATA_PATH OBS_INSTALL_PREFIX "/" OBS_DATA_PATH "/obs-studio/"

static inline bool
check_path(const char* data, const char* path, std::string& output)
{
	std::ostringstream str;
	str << path << data;
	output = str.str();

	blog(LOG_DEBUG, "Attempted path: %s", output.c_str());

	return (access(output.c_str(), R_OK) == 0);
}

bool
GetDataFilePath(const char* data, std::string& path)
{
	char* dataPath = getenv("OBS_DATA_PATH");
	if (dataPath != NULL) {
		if (check_path(data, dataPath, path))
			return true;
	}

	char* relativeDataPath =
		os_get_executable_path_ptr("../" OBS_DATA_PATH "/obs-studio/");

	if (relativeDataPath) {
		bool result = check_path(data, relativeDataPath, path);
		bfree(relativeDataPath);

		if (result)
			return true;
	}

	if (check_path(data, OBS_DATA_PATH "/obs-studio/", path))
		return true;

	if (check_path(data, INSTALL_DATA_PATH, path))
		return true;

	return false;
}

std::string GetDefaultVideoSavePath()
{
	BPath videoPath;
	status_t result = find_directory(B_USER_DIRECTORY, &videoPath);

	if (result != B_OK)
		return getenv("HOME");

	return videoPath.Path();
}

std::vector<std::string>
GetPreferredLocales()
{
	auto localeRoster = BLocaleRoster::Default();
	if (localeRoster == nullptr)
		return {};

	localeRoster->Refresh();

	BMessage languagesMsg;
	if (localeRoster->GetPreferredLanguages(&languagesMsg) != B_OK)
		return {};

	BStringList haikuLanguages;
	if (languagesMsg.FindStrings("language", &haikuLanguages) != B_OK)
		return {};

	auto obsLocales = GetLocaleNames();

	auto haiku_to_obs = [&obsLocales](std::string haiku) {
		std::string langMatch;

		for (const auto& localePair : obsLocales) {
			const auto& locale = localePair.first;
			if (locale == haiku.substr(0, locale.size()))
				return locale;

			if (langMatch.size())
				continue;

			if (locale.substr(0, 2) == haiku.substr(0, 2))
				langMatch = locale;
		}

		return langMatch;
	};

	std::vector<std::string> matchedLanguages;
	matchedLanguages.reserve(obsLocales.size());

	for (int32 index = 0; index < haikuLanguages.CountStrings(); index++) {
		BString language = haikuLanguages.StringAt(index);

		std::string locale = haiku_to_obs(language.String());
		if (locale.empty())
			continue;

		if (find(begin(matchedLanguages), end(matchedLanguages), locale) != end(matchedLanguages))
            continue;

        matchedLanguages.emplace_back(locale);
	}

	return matchedLanguages;
}

bool
IsAlwaysOnTop(QWidget* window)
{
	return (window->windowFlags() & Qt::WindowStaysOnTopHint) != 0;
}

void
SetAlwaysOnTop(QWidget* window, bool enable)
{
	Qt::WindowFlags flags = window->windowFlags();

	if (enable)
		flags |= Qt::WindowStaysOnTopHint;
	else
		flags &= ~Qt::WindowStaysOnTopHint;

	window->setWindowFlags(flags);
	window->show();
}

bool
SetDisplayAffinitySupported(void)
{
    // Not implemented yet
    return false;
}

bool
HighContrastEnabled()
{
    // Not implemented yet
    return false;
}

void
TaskbarOverlayInit()
{
    // Not implemented yet
    return;
}

void
TaskbarOverlaySetStatus(TaskbarOverlayStatus status)
{
	UNUSED_PARAMETER(status);
	// Not implemented yet
	return;
}


void
CheckIfAlreadyRunning(bool& already_running)
{
	// Not implemented yet
	already_running = false;
}
