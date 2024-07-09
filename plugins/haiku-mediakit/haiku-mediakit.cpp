/* haiku-mediakit.cpp
 *
 * Copyright 2024 Jacob Secunda <secundaja@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <obs-module.h>


OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("haiku-mediakit", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Haiku MediaKit input/output capture";
}

extern struct obs_source_info haiku_input_capture;
extern struct obs_source_info haiku_output_capture;

bool obs_module_load(void)
{
	obs_register_source(&haiku_input_capture);
	obs_register_source(&haiku_output_capture);
	return true;
}

void obs_module_unload(void)
{

}
