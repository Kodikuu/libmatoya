// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <android/keycodes.h>
#include <android/input.h>

static const MTY_Key APP_KEYS[] = {
	[AKEYCODE_ESCAPE] = MTY_KEY_ESCAPE,
	[AKEYCODE_1] = MTY_KEY_1,
	[AKEYCODE_2] = MTY_KEY_2,
	[AKEYCODE_3] = MTY_KEY_3,
	[AKEYCODE_4] = MTY_KEY_4,
	[AKEYCODE_5] = MTY_KEY_5,
	[AKEYCODE_6] = MTY_KEY_6,
	[AKEYCODE_7] = MTY_KEY_7,
	[AKEYCODE_8] = MTY_KEY_8,
	[AKEYCODE_9] = MTY_KEY_9,
	[AKEYCODE_0] = MTY_KEY_0,
	[AKEYCODE_MINUS] = MTY_KEY_MINUS,
	[AKEYCODE_EQUALS] = MTY_KEY_EQUALS,
	[AKEYCODE_DEL] = MTY_KEY_BACKSPACE,
	[AKEYCODE_TAB] = MTY_KEY_TAB,
	[AKEYCODE_Q] = MTY_KEY_Q,
	// [0] = MTY_KEY_AUDIO_PREV,
	[AKEYCODE_W] = MTY_KEY_W,
	[AKEYCODE_E] = MTY_KEY_E,
	[AKEYCODE_R] = MTY_KEY_R,
	[AKEYCODE_T] = MTY_KEY_T,
	[AKEYCODE_Y] = MTY_KEY_Y,
	[AKEYCODE_U] = MTY_KEY_U,
	[AKEYCODE_I] = MTY_KEY_I,
	[AKEYCODE_O] = MTY_KEY_O,
	[AKEYCODE_P] = MTY_KEY_P,
	// [0] = MTY_KEY_AUDIO_NEXT,
	[AKEYCODE_LEFT_BRACKET] = MTY_KEY_LBRACKET,
	[AKEYCODE_RIGHT_BRACKET] = MTY_KEY_RBRACKET,
	[AKEYCODE_ENTER] = MTY_KEY_ENTER,
	[AKEYCODE_NUMPAD_ENTER] = MTY_KEY_NP_ENTER,
	[AKEYCODE_CTRL_LEFT] = MTY_KEY_LCTRL,
	[AKEYCODE_CTRL_RIGHT] = MTY_KEY_RCTRL,
	[AKEYCODE_A] = MTY_KEY_A,
	[AKEYCODE_S] = MTY_KEY_S,
	[AKEYCODE_D] = MTY_KEY_D,
	// [0] = MTY_KEY_MUTE,
	[AKEYCODE_F] = MTY_KEY_F,
	[AKEYCODE_G] = MTY_KEY_G,
	// [0] = MTY_KEY_AUDIO_PLAY,
	[AKEYCODE_H] = MTY_KEY_H,
	[AKEYCODE_J] = MTY_KEY_J,
	// [0] = MTY_KEY_AUDIO_STOP,
	[AKEYCODE_K] = MTY_KEY_K,
	[AKEYCODE_L] = MTY_KEY_L,
	[AKEYCODE_SEMICOLON] = MTY_KEY_SEMICOLON,
	[AKEYCODE_APOSTROPHE] = MTY_KEY_QUOTE,
	[AKEYCODE_GRAVE] = MTY_KEY_GRAVE,
	[AKEYCODE_SHIFT_LEFT] = MTY_KEY_LSHIFT,
	[AKEYCODE_BACKSLASH] = MTY_KEY_BACKSLASH,
	[AKEYCODE_Z] = MTY_KEY_Z,
	[AKEYCODE_X] = MTY_KEY_X,
	[AKEYCODE_C] = MTY_KEY_C,
	// [0] = MTY_KEY_VOLUME_DOWN,
	[AKEYCODE_V] = MTY_KEY_V,
	[AKEYCODE_B] = MTY_KEY_B,
	// [0] = MTY_KEY_VOLUME_UP,
	[AKEYCODE_N] = MTY_KEY_N,
	[AKEYCODE_M] = MTY_KEY_M,
	[AKEYCODE_COMMA] = MTY_KEY_COMMA,
	[AKEYCODE_PERIOD] = MTY_KEY_PERIOD,
	[AKEYCODE_SLASH] = MTY_KEY_SLASH,
	[AKEYCODE_NUMPAD_DIVIDE] = MTY_KEY_NP_DIVIDE,
	[AKEYCODE_SHIFT_RIGHT] = MTY_KEY_RSHIFT,
	[AKEYCODE_NUMPAD_MULTIPLY] = MTY_KEY_NP_MULTIPLY,
	// [0] = MTY_KEY_PRINT_SCREEN,
	[AKEYCODE_ALT_LEFT] = MTY_KEY_LALT,
	[AKEYCODE_ALT_RIGHT] = MTY_KEY_RALT,
	[AKEYCODE_SPACE] = MTY_KEY_SPACE,
	[AKEYCODE_CAPS_LOCK] = MTY_KEY_CAPS,
	[AKEYCODE_F1] = MTY_KEY_F1,
	[AKEYCODE_F2] = MTY_KEY_F2,
	[AKEYCODE_F3] = MTY_KEY_F3,
	[AKEYCODE_F4] = MTY_KEY_F4,
	[AKEYCODE_F5] = MTY_KEY_F5,
	[AKEYCODE_F6] = MTY_KEY_F6,
	[AKEYCODE_F7] = MTY_KEY_F7,
	[AKEYCODE_F8] = MTY_KEY_F8,
	[AKEYCODE_F9] = MTY_KEY_F9,
	[AKEYCODE_F10] = MTY_KEY_F10,
	[AKEYCODE_NUM_LOCK] = MTY_KEY_NUM_LOCK,
	[AKEYCODE_SCROLL_LOCK] = MTY_KEY_SCROLL_LOCK,
	// [0] = MTY_KEY_PAUSE,
	[AKEYCODE_NUMPAD_7] = MTY_KEY_NP_7,
	[AKEYCODE_MOVE_HOME] = MTY_KEY_HOME,
	[AKEYCODE_NUMPAD_8] = MTY_KEY_NP_8,
	[AKEYCODE_DPAD_UP] = MTY_KEY_UP,
	[AKEYCODE_NUMPAD_9] = MTY_KEY_NP_9,
	[AKEYCODE_PAGE_UP] = MTY_KEY_PAGE_UP,
	[AKEYCODE_NUMPAD_SUBTRACT] = MTY_KEY_NP_MINUS,
	[AKEYCODE_NUMPAD_4] = MTY_KEY_NP_4,
	[AKEYCODE_DPAD_LEFT] = MTY_KEY_LEFT,
	[AKEYCODE_NUMPAD_5] = MTY_KEY_NP_5,
	[AKEYCODE_NUMPAD_6] = MTY_KEY_NP_6,
	[AKEYCODE_DPAD_RIGHT] = MTY_KEY_RIGHT,
	[AKEYCODE_NUMPAD_ADD] = MTY_KEY_NP_PLUS,
	[AKEYCODE_NUMPAD_1] = MTY_KEY_NP_1,
	[AKEYCODE_MOVE_END] = MTY_KEY_END,
	[AKEYCODE_NUMPAD_2] = MTY_KEY_NP_2,
	[AKEYCODE_DPAD_DOWN] = MTY_KEY_DOWN,
	[AKEYCODE_NUMPAD_3] = MTY_KEY_NP_3,
	[AKEYCODE_PAGE_DOWN] = MTY_KEY_PAGE_DOWN,
	[AKEYCODE_NUMPAD_0] = MTY_KEY_NP_0,
	[AKEYCODE_INSERT] = MTY_KEY_INSERT,
	[AKEYCODE_NUMPAD_DOT] = MTY_KEY_NP_PERIOD,
	[AKEYCODE_FORWARD_DEL] = MTY_KEY_DELETE,
	// [0] = MTY_KEY_INTL_BACKSLASH,
	[AKEYCODE_F11] = MTY_KEY_F11,
	[AKEYCODE_F12] = MTY_KEY_F12,
	[AKEYCODE_META_LEFT] = MTY_KEY_LWIN,
	[AKEYCODE_META_RIGHT] = MTY_KEY_RWIN,
	// [0] = MTY_KEY_APP,
	// [0] = MTY_KEY_F13,
	// [0] = MTY_KEY_F14,
	// [0] = MTY_KEY_F15,
	// [0] = MTY_KEY_F16,
	// [0] = MTY_KEY_F17,
	// [0] = MTY_KEY_F18,
	// [0] = MTY_KEY_F19,
	// [0] = MTY_KEY_MEDIA_SELECT,
	[AKEYCODE_KATAKANA_HIRAGANA] = MTY_KEY_JP,
	[AKEYCODE_RO] = MTY_KEY_RO,
	[AKEYCODE_HENKAN] = MTY_KEY_HENKAN,
	[AKEYCODE_MUHENKAN] = MTY_KEY_MUHENKAN,
	// [0] = MTY_KEY_INTL_COMMA,
	[AKEYCODE_YEN] = MTY_KEY_YEN,
};

#define APP_KEYS_MAX (sizeof(APP_KEYS) / sizeof(MTY_Key))

static MTY_Mod app_keymods(int32_t mods)
{
	MTY_Mod mty = MTY_MOD_NONE;

	if (mods & AMETA_SHIFT_LEFT_ON)  mty |= MTY_MOD_LSHIFT;
	if (mods & AMETA_SHIFT_RIGHT_ON) mty |= MTY_MOD_RSHIFT;
	if (mods & AMETA_CTRL_LEFT_ON)   mty |= MTY_MOD_LCTRL;
	if (mods & AMETA_CTRL_RIGHT_ON)  mty |= MTY_MOD_RCTRL;
	if (mods & AMETA_ALT_LEFT_ON)    mty |= MTY_MOD_LALT;
	if (mods & AMETA_ALT_RIGHT_ON)   mty |= MTY_MOD_RALT;
	if (mods & AMETA_META_LEFT_ON)   mty |= MTY_MOD_LWIN;
	if (mods & AMETA_META_RIGHT_ON)  mty |= MTY_MOD_RWIN;
	if (mods & AMETA_CAPS_LOCK_ON)   mty |= MTY_MOD_CAPS;
	if (mods & AMETA_NUM_LOCK_ON)    mty |= MTY_MOD_NUM;

	return mty;
}
