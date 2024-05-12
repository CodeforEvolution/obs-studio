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

#include "obs-internal.h"


// Module Loading

const char *get_module_extension(void)
{
	return ".so";
}

static const char *module_bin[] = {
	"../../obs-plugins/64bit",
	OBS_INSTALL_PREFIX "/" OBS_PLUGIN_DESTINATION,
};

static const char *module_data[] = {
	OBS_DATA_PATH "/obs-plugins/%module%",
	OBS_INSTALL_DATA_PATH "/obs-plugins/%module%",
};

static const int module_patterns_size =
	sizeof(module_bin) / sizeof(module_bin[0]);

void add_default_module_paths(void)
{
	char *module_bin_path =
		os_get_executable_path_ptr("../" OBS_PLUGIN_PATH);
	char *module_data_path = os_get_executable_path_ptr(
		"../" OBS_DATA_PATH "/obs-plugins/%module%");

	if (module_bin_path && module_data_path) {
		char *abs_module_bin_path =
			os_get_abs_path_ptr(module_bin_path);

		if (abs_module_bin_path &&
		    strcmp(abs_module_bin_path, OBS_INSTALL_PREFIX
			   "/" OBS_PLUGIN_DESTINATION) != 0) {
			obs_add_module_path(module_bin_path, module_data_path);
		}
		bfree(abs_module_bin_path);
	}

	bfree(module_bin_path);
	bfree(module_data_path);

	for (int i = 0; i < module_patterns_size; i++) {
		obs_add_module_path(module_bin[i], module_data[i]);
	}
}

/*
 *   /boot/system/non-packaged/data/libobs
 *   /boot/system/data/libobs
 */
char *find_libobs_data_file(const char *file)
{
	struct dstr output;
	dstr_init(&output);

	if (check_path(file, OBS_DATA_PATH "/libobs/", &output))
		return output.array;

	char *relative_data_path =
		os_get_executable_path_ptr("../" OBS_DATA_PATH "/libobs/");
	if (relative_data_path) {
		bool found = check_path(file, relative_data_path, &output);

		bfree(relative_data_path);

		if (found) {
			return output.array;
		}
	}

	if (OBS_INSTALL_PREFIX[0] != 0) {
		if (check_path(file, OBS_INSTALL_DATA_PATH "/libobs/", &output))
			return output.array;
	}

	dstr_free(&output);
	return NULL;
}



// Logging

static void log_processor_name(void)
{

}

static void log_processor_speed(void)
{

}

static void log_model_name(void)
{

}

static void log_processor_cores(void)
{

}

static void log_emulation_status(void)
{

}

static void log_available_memory(void)
{

}

static void log_os(void)
{

}

static void log_kernel_version(void)
{

}

// Platform Hook
void log_system_info(void)
{

}


// Hotkeys
struct obs_hotkeys_platform {
    bool is_key_down[OBS_KEY_LAST_VALUE];
};

#define INVALID_KEY 0xff

int
obs_key_to_haiku_key(obs_key_t code)
{
    UNUSED_PARAMETER(code);
 	// switch (key) {
		// case OBS_KEY_RETURN:
			// return B_RETURN;
		// case OBS_KEY_ESCAPE:
			// return B_ESCAPE;
		// case OBS_KEY_TAB:
			// return B_TAB;
		// case OBS_KEY_BACKSPACE:
			// return B_BACKSPACE;
		// case OBS_KEY_INSERT:
			// return B_INSERT;
		// case OBS_KEY_DELETE:
			// return B_DELETE;
		// case OBS_KEY_PAUSE:
			// return B_PAUSE_KEY;
		// case OBS_KEY_PRINT:
			// return B_PRINT_KEY;
		// case OBS_KEY_HOME:
			// return B_HOME;
		// case OBS_KEY_END:
			// return B_END;
		// case OBS_KEY_LEFT:
			// return B_LEFT_ARROW;
		// case OBS_KEY_UP:
			// return B_UP_ARROW;
		// case OBS_KEY_RIGHT:
			// return B_RIGHT_ARROW;
		// case OBS_KEY_DOWN:
			// return B_DOWN_ARROW;
		// case OBS_KEY_PAGEUP:
			// return B_PAGE_UP;
		// case OBS_KEY_PAGEDOWN:
			// return B_PAGE_DOWN;
// 
		// case OBS_KEY_SHIFT:
			// return B_SHIFT_KEY;
		// case OBS_KEY_CONTROL:
			// return B_CONTROL_KEY;
		// case OBS_KEY_ALT:
			// return B_OPTION_KEY;
		// case OBS_KEY_CAPSLOCK:
			// return B_CAPS_LOCK;
		// case OBS_KEY_NUMLOCK:
			// return B_NUM_LOCK;
		// case OBS_KEY_SCROLLLOCK:
			// return B_SCROLL_LOCK;
// 
		// case OBS_KEY_F1:
			// return B_F1_KEY;
		// case OBS_KEY_F2:
			// return B_F2_KEY;
		// case OBS_KEY_F3:
			// return B_F3_KEY;
		// case OBS_KEY_F4:
			// return B_F4_KEY;
		// case OBS_KEY_F5:
			// return B_F5_KEY;
		// case OBS_KEY_F6:
			// return B_F6_KEY;
		// case OBS_KEY_F7:
			// return B_F7_KEY;
		// case OBS_KEY_F8:
			// return B_F8_KEY;
		// case OBS_KEY_F9:
			// return B_F9_KEY;
		// case OBS_KEY_F10:
			// return B_F10_KEY;
		// case OBS_KEY_F11:
			// return B_F11_KEY;
		// case OBS_KEY_F12:
			// return B_F12_KEY;
		// case OBS_KEY_F13:
		// case OBS_KEY_F14:
		// case OBS_KEY_F15:
		// case OBS_KEY_F16:
		// case OBS_KEY_F17:
		// case OBS_KEY_F18:
		// case OBS_KEY_F19:
		// case OBS_KEY_F20:
		// case OBS_KEY_F21:
		// case OBS_KEY_F22:
		// case OBS_KEY_F23:
		// case OBS_KEY_F24:
		// case OBS_KEY_F25:
		// case OBS_KEY_F26:
		// case OBS_KEY_F27:
		// case OBS_KEY_F28:
		// case OBS_KEY_F29:
		// case OBS_KEY_F30:
		// case OBS_KEY_F31:
		// case OBS_KEY_F32:
		// case OBS_KEY_F33:
		// case OBS_KEY_F34:
		// case OBS_KEY_F35:
			// return INVALID_KEY; // TODO
// 
		// case OBS_KEY_MENU:
			// return B_MENU_KEY;
// 
		// case OBS_KEY_HYPER_L:
		// case OBS_KEY_HYPER_R:
			// return INVALID_KEY; // TODO
// 
		// case OBS_KEY_HELP:
		// case OBS_KEY_CANCEL:
		// case OBS_KEY_FIND:
		// case OBS_KEY_REDO:
		// case OBS_KEY_UNDO:
			// return INVALID_KEY; // TODO		
		// 
		// case OBS_KEY_SPACE:
			// return B_SPACE;
// 
		// case OBS_KEY_COPY:
		// case OBS_KEY_CUT:
		// case OBS_KEY_OPEN:
		// case OBS_KEY_PASTE:
		// case OBS_KEY_FRONT:
		// case OBS_KEY_PROPS:
// 
		// case OBS_KEY_EXCLAM:
			// return XK_exclam;
		// case OBS_KEY_QUOTEDBL:
			// return XK_quotedbl;
		// case OBS_KEY_NUMBERSIGN:
			// return XK_numbersign;
		// case OBS_KEY_DOLLAR:
			// return XK_dollar;
		// case OBS_KEY_PERCENT:
			// return XK_percent;
		// case OBS_KEY_AMPERSAND:
			// return XK_ampersand;
		// case OBS_KEY_APOSTROPHE:
			// return XK_apostrophe;
		// case OBS_KEY_PARENLEFT:
			// return XK_parenleft;
		// case OBS_KEY_PARENRIGHT:
			// return XK_parenright;
		// case OBS_KEY_ASTERISK:
			// return XK_asterisk;
		// case OBS_KEY_PLUS:
			// return XK_plus;
		// case OBS_KEY_COMMA:
			// return XK_comma;
		// case OBS_KEY_MINUS:
			// return XK_minus;
		// case OBS_KEY_PERIOD:
			// return XK_period;
		// case OBS_KEY_SLASH:
			// return XK_slash;
		// case OBS_KEY_0:
			// return XK_0;
		// case OBS_KEY_1:
			// return XK_1;
		// case OBS_KEY_2:
			// return XK_2;
		// case OBS_KEY_3:
			// return XK_3;
		// case OBS_KEY_4:
			// return XK_4;
		// case OBS_KEY_5:
			// return XK_5;
		// case OBS_KEY_6:
			// return XK_6;
		// case OBS_KEY_7:
			// return XK_7;
		// case OBS_KEY_8:
			// return XK_8;
		// case OBS_KEY_9:
			// return XK_9;
		// case OBS_KEY_NUMEQUAL:
			// return XK_KP_Equal;
		// case OBS_KEY_NUMASTERISK:
			// return XK_KP_Multiply;
		// case OBS_KEY_NUMPLUS:
			// return XK_KP_Add;
		// case OBS_KEY_NUMCOMMA:
			// return XK_KP_Separator;
		// case OBS_KEY_NUMMINUS:
			// return XK_KP_Subtract;
		// case OBS_KEY_NUMPERIOD:
			// return XK_KP_Decimal;
		// case OBS_KEY_NUMSLASH:
			// return XK_KP_Divide;
		// case OBS_KEY_NUM0:
			// return XK_KP_0;
		// case OBS_KEY_NUM1:
			// return XK_KP_1;
		// case OBS_KEY_NUM2:
			// return XK_KP_2;
		// case OBS_KEY_NUM3:
			// return XK_KP_3;
		// case OBS_KEY_NUM4:
			// return XK_KP_4;
		// case OBS_KEY_NUM5:
			// return XK_KP_5;
		// case OBS_KEY_NUM6:
			// return XK_KP_6;
		// case OBS_KEY_NUM7:
			// return XK_KP_7;
		// case OBS_KEY_NUM8:
			// return XK_KP_8;
		// case OBS_KEY_NUM9:
			// return XK_KP_9;
		// case OBS_KEY_COLON:
			// return XK_colon;
		// case OBS_KEY_SEMICOLON:
			// return XK_semicolon;
		// case OBS_KEY_LESS:
			// return XK_less;
		// case OBS_KEY_EQUAL:
			// return XK_equal;
		// case OBS_KEY_GREATER:
			// return XK_greater;
		// case OBS_KEY_QUESTION:
			// return XK_question;
		// case OBS_KEY_AT:
			// return XK_at;
		// case OBS_KEY_A:
			// return XK_A;
		// case OBS_KEY_B:
			// return XK_B;
		// case OBS_KEY_C:
			// return XK_C;
		// case OBS_KEY_D:
			// return XK_D;
		// case OBS_KEY_E:
			// return XK_E;
		// case OBS_KEY_F:
			// return XK_F;
		// case OBS_KEY_G:
			// return XK_G;
		// case OBS_KEY_H:
			// return XK_H;
		// case OBS_KEY_I:
			// return XK_I;
		// case OBS_KEY_J:
			// return XK_J;
		// case OBS_KEY_K:
			// return XK_K;
		// case OBS_KEY_L:
			// return XK_L;
		// case OBS_KEY_M:
			// return XK_M;
		// case OBS_KEY_N:
			// return XK_N;
		// case OBS_KEY_O:
			// return XK_O;
		// case OBS_KEY_P:
			// return XK_P;
		// case OBS_KEY_Q:
			// return XK_Q;
		// case OBS_KEY_R:
			// return XK_R;
		// case OBS_KEY_S:
			// return XK_S;
		// case OBS_KEY_T:
			// return XK_T;
		// case OBS_KEY_U:
			// return XK_U;
		// case OBS_KEY_V:
			// return XK_V;
		// case OBS_KEY_W:
			// return XK_W;
		// case OBS_KEY_X:
			// return XK_X;
		// case OBS_KEY_Y:
			// return XK_Y;
		// case OBS_KEY_Z:
			// return XK_Z;
		// case OBS_KEY_BRACKETLEFT:
			// return XK_bracketleft;
		// case OBS_KEY_BACKSLASH:
			// return XK_backslash;
		// case OBS_KEY_BRACKETRIGHT:
			// return XK_bracketright;
		// case OBS_KEY_ASCIICIRCUM:
			// return XK_asciicircum;
		// case OBS_KEY_UNDERSCORE:
			// return XK_underscore;
		// case OBS_KEY_QUOTELEFT:
			// return XK_quoteleft;
		// case OBS_KEY_BRACELEFT:
			// return XK_braceleft;
		// case OBS_KEY_BAR:
			// return XK_bar;
		// case OBS_KEY_BRACERIGHT:
			// return XK_braceright;
		// case OBS_KEY_ASCIITILDE:
			// return XK_grave;
		// case OBS_KEY_NOBREAKSPACE:
			// return XK_nobreakspace;
		// case OBS_KEY_EXCLAMDOWN:
			// return XK_exclamdown;
		// case OBS_KEY_CENT:
			// return XK_cent;
		// case OBS_KEY_STERLING:
			// return XK_sterling;
		// case OBS_KEY_CURRENCY:
			// return XK_currency;
		// case OBS_KEY_YEN:
			// return XK_yen;
		// case OBS_KEY_BROKENBAR:
			// return XK_brokenbar;
		// case OBS_KEY_SECTION:
			// return XK_section;
		// case OBS_KEY_DIAERESIS:
			// return XK_diaeresis;
		// case OBS_KEY_COPYRIGHT:
			// return XK_copyright;
		// case OBS_KEY_ORDFEMININE:
			// return XK_ordfeminine;
		// case OBS_KEY_GUILLEMOTLEFT:
			// return XK_guillemotleft;
		// case OBS_KEY_NOTSIGN:
			// return XK_notsign;
		// case OBS_KEY_HYPHEN:
			// return XK_hyphen;
		// case OBS_KEY_REGISTERED:
			// return XK_registered;
		// case OBS_KEY_MACRON:
			// return XK_macron;
		// case OBS_KEY_DEGREE:
			// return XK_degree;
		// case OBS_KEY_PLUSMINUS:
			// return XK_plusminus;
		// case OBS_KEY_TWOSUPERIOR:
			// return XK_twosuperior;
		// case OBS_KEY_THREESUPERIOR:
			// return XK_threesuperior;
		// case OBS_KEY_ACUTE:
			// return XK_acute;
		// case OBS_KEY_MU:
			// return XK_mu;
		// case OBS_KEY_PARAGRAPH:
			// return XK_paragraph;
		// case OBS_KEY_PERIODCENTERED:
			// return XK_periodcentered;
		// case OBS_KEY_CEDILLA:
			// return XK_cedilla;
		// case OBS_KEY_ONESUPERIOR:
			// return XK_onesuperior;
		// case OBS_KEY_MASCULINE:
			// return XK_masculine;
		// case OBS_KEY_GUILLEMOTRIGHT:
			// return XK_guillemotright;
		// case OBS_KEY_ONEQUARTER:
			// return XK_onequarter;
		// case OBS_KEY_ONEHALF:
			// return XK_onehalf;
		// case OBS_KEY_THREEQUARTERS:
			// return XK_threequarters;
		// case OBS_KEY_QUESTIONDOWN:
			// return XK_questiondown;
		// case OBS_KEY_AGRAVE:
			// return XK_Agrave;
		// case OBS_KEY_AACUTE:
			// return XK_Aacute;
		// case OBS_KEY_ACIRCUMFLEX:
			// return XK_Acircumflex;
		// case OBS_KEY_ATILDE:
			// return XK_Atilde;
		// case OBS_KEY_ADIAERESIS:
			// return XK_Adiaeresis;
		// case OBS_KEY_ARING:
			// return XK_Aring;
		// case OBS_KEY_AE:
			// return XK_AE;
		// case OBS_KEY_CCEDILLA:
			// return XK_cedilla;
		// case OBS_KEY_EGRAVE:
			// return XK_Egrave;
		// case OBS_KEY_EACUTE:
			// return XK_Eacute;
		// case OBS_KEY_ECIRCUMFLEX:
			// return XK_Ecircumflex;
		// case OBS_KEY_EDIAERESIS:
			// return XK_Ediaeresis;
		// case OBS_KEY_IGRAVE:
			// return XK_Igrave;
		// case OBS_KEY_IACUTE:
			// return XK_Iacute;
		// case OBS_KEY_ICIRCUMFLEX:
			// return XK_Icircumflex;
		// case OBS_KEY_IDIAERESIS:
			// return XK_Idiaeresis;
		// case OBS_KEY_ETH:
			// return XK_ETH;
		// case OBS_KEY_NTILDE:
			// return XK_Ntilde;
		// case OBS_KEY_OGRAVE:
			// return XK_Ograve;
		// case OBS_KEY_OACUTE:
			// return XK_Oacute;
		// case OBS_KEY_OCIRCUMFLEX:
			// return XK_Ocircumflex;
		// case OBS_KEY_ODIAERESIS:
			// return XK_Odiaeresis;
		// case OBS_KEY_MULTIPLY:
			// return XK_multiply;
		// case OBS_KEY_OOBLIQUE:
			// return XK_Ooblique;
		// case OBS_KEY_UGRAVE:
			// return XK_Ugrave;
		// case OBS_KEY_UACUTE:
			// return XK_Uacute;
		// case OBS_KEY_UCIRCUMFLEX:
			// return XK_Ucircumflex;
		// case OBS_KEY_UDIAERESIS:
			// return XK_Udiaeresis;
		// case OBS_KEY_YACUTE:
			// return XK_Yacute;
		// case OBS_KEY_THORN:
			// return XK_Thorn;
		// case OBS_KEY_SSHARP:
			// return XK_ssharp;
		// case OBS_KEY_DIVISION:
			// return XK_division;
		// case OBS_KEY_YDIAERESIS:
			// return XK_Ydiaeresis;
		// case OBS_KEY_MULTI_KEY:
			// return XK_Multi_key;
		// case OBS_KEY_CODEINPUT:
			// return XK_Codeinput;
		// case OBS_KEY_SINGLECANDIDATE:
			// return XK_SingleCandidate;
		// case OBS_KEY_MULTIPLECANDIDATE:
			// return XK_MultipleCandidate;
		// case OBS_KEY_PREVIOUSCANDIDATE:
			// return XK_PreviousCandidate;
		// case OBS_KEY_MODE_SWITCH:
			// return XK_Mode_switch;
		// case OBS_KEY_KANJI:
			// return XK_Kanji;
		// case OBS_KEY_MUHENKAN:
			// return XK_Muhenkan;
		// case OBS_KEY_HENKAN:
			// return XK_Henkan;
		// case OBS_KEY_ROMAJI:
			// return XK_Romaji;
		// case OBS_KEY_HIRAGANA:
			// return XK_Hiragana;
		// case OBS_KEY_KATAKANA:
			// return XK_Katakana;
		// case OBS_KEY_HIRAGANA_KATAKANA:
			// return XK_Hiragana_Katakana;
		// case OBS_KEY_ZENKAKU:
			// return XK_Zenkaku;
		// case OBS_KEY_HANKAKU:
			// return XK_Hankaku;
		// case OBS_KEY_ZENKAKU_HANKAKU:
			// return XK_Zenkaku_Hankaku;
		// case OBS_KEY_TOUROKU:
			// return XK_Touroku;
		// case OBS_KEY_MASSYO:
			// return XK_Massyo;
		// case OBS_KEY_KANA_LOCK:
			// return XK_Kana_Lock;
		// case OBS_KEY_KANA_SHIFT:
			// return XK_Kana_Shift;
		// case OBS_KEY_EISU_SHIFT:
			// return XK_Eisu_Shift;
		// case OBS_KEY_EISU_TOGGLE:
			// return XK_Eisu_toggle;
		// case OBS_KEY_HANGUL:
			// return XK_Hangul;
		// case OBS_KEY_HANGUL_START:
			// return XK_Hangul_Start;
		// case OBS_KEY_HANGUL_END:
			// return XK_Hangul_End;
		// case OBS_KEY_HANGUL_HANJA:
			// return XK_Hangul_Hanja;
		// case OBS_KEY_HANGUL_JAMO:
			// return XK_Hangul_Jamo;
		// case OBS_KEY_HANGUL_ROMAJA:
			// return XK_Hangul_Romaja;
		// case OBS_KEY_HANGUL_BANJA:
			// return XK_Hangul_Banja;
		// case OBS_KEY_HANGUL_PREHANJA:
			// return XK_Hangul_PreHanja;
		// case OBS_KEY_HANGUL_POSTHANJA:
			// return XK_Hangul_PostHanja;
		// case OBS_KEY_HANGUL_SPECIAL:
			// return XK_Hangul_Special;
		// case OBS_KEY_DEAD_GRAVE:
			// return XK_dead_grave;
		// case OBS_KEY_DEAD_ACUTE:
			// return XK_dead_acute;
		// case OBS_KEY_DEAD_CIRCUMFLEX:
			// return XK_dead_circumflex;
		// case OBS_KEY_DEAD_TILDE:
			// return XK_dead_tilde;
		// case OBS_KEY_DEAD_MACRON:
			// return XK_dead_macron;
		// case OBS_KEY_DEAD_BREVE:
			// return XK_dead_breve;
		// case OBS_KEY_DEAD_ABOVEDOT:
			// return XK_dead_abovedot;
		// case OBS_KEY_DEAD_DIAERESIS:
			// return XK_dead_diaeresis;
		// case OBS_KEY_DEAD_ABOVERING:
			// return XK_dead_abovering;
		// case OBS_KEY_DEAD_DOUBLEACUTE:
			// return XK_dead_doubleacute;
		// case OBS_KEY_DEAD_CARON:
			// return XK_dead_caron;
		// case OBS_KEY_DEAD_CEDILLA:
			// return XK_dead_cedilla;
		// case OBS_KEY_DEAD_OGONEK:
			// return XK_dead_ogonek;
		// case OBS_KEY_DEAD_IOTA:
			// return XK_dead_iota;
		// case OBS_KEY_DEAD_VOICED_SOUND:
			// return XK_dead_voiced_sound;
		// case OBS_KEY_DEAD_SEMIVOICED_SOUND:
			// return XK_dead_semivoiced_sound;
		// case OBS_KEY_DEAD_BELOWDOT:
			// return XK_dead_belowdot;
		// case OBS_KEY_DEAD_HOOK:
			// return XK_dead_hook;
		// case OBS_KEY_DEAD_HORN:
			// return XK_dead_horn;
// 
		// case OBS_KEY_MOUSE1:
			// return MOUSE_1;
		// case OBS_KEY_MOUSE2:
			// return MOUSE_2;
		// case OBS_KEY_MOUSE3:
			// return MOUSE_3;
		// case OBS_KEY_MOUSE4:
			// return MOUSE_4;
		// case OBS_KEY_MOUSE5:
			// return MOUSE_5;
// 
		// case OBS_KEY_VK_MEDIA_PLAY_PAUSE:
			// return XF86XK_AudioPlay;
		// case OBS_KEY_VK_MEDIA_STOP:
			// return XF86XK_AudioStop;
		// case OBS_KEY_VK_MEDIA_PREV_TRACK:
			// return XF86XK_AudioPrev;
		// case OBS_KEY_VK_MEDIA_NEXT_TRACK:
			// return XF86XK_AudioNext;
		// case OBS_KEY_VK_VOLUME_MUTE:
			// return XF86XK_AudioMute;
		// case OBS_KEY_VK_VOLUME_DOWN:
			// return XF86XK_AudioRaiseVolume;
		// case OBS_KEY_VK_VOLUME_UP:
			// return XF86XK_AudioLowerVolume;
// 
		// /* TODO: Implement keys for non-US keyboards */
		// default:
			// ;
	// }

	return 0;   
}

obs_key_t obs_key_from_virtual_key(int code)
{
    UNUSED_PARAMETER(code);
	return INVALID_KEY;
}

void obs_key_to_str(obs_key_t key, struct dstr *str)
{
    UNUSED_PARAMETER(key);
    UNUSED_PARAMETER(str);
}

void obs_key_combination_to_str(obs_key_combination_t key, struct dstr *str)
{
    UNUSED_PARAMETER(key);
    UNUSED_PARAMETER(str);
}

bool obs_hotkeys_platform_init(struct obs_core_hotkeys *hotkeys)
{
    UNUSED_PARAMETER(hotkeys);
	return true;
}

void obs_hotkeys_platform_free(struct obs_core_hotkeys *hotkeys)
{
    UNUSED_PARAMETER(hotkeys);
}


bool obs_hotkeys_platform_is_pressed(obs_hotkeys_platform_t *plat, obs_key_t key)
{
    UNUSED_PARAMETER(plat);
    UNUSED_PARAMETER(key);
	return false;
}
