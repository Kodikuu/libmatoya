// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include "matoya.h"

#define kVK_WinApp 0x6E

enum {
	NS_MOD_LCTRL  = 0x000001,
	NS_MOD_LSHIFT = 0x000002,
	NS_MOD_RSHIFT = 0x000004,
	NS_MOD_LCMD   = 0x000008,
	NS_MOD_RCMD   = 0x000010,
	NS_MOD_LALT   = 0x000020,
	NS_MOD_RALT   = 0x000040,
	NS_MOD_RCTRL  = 0x002000,
};

static MTY_Mod window_modifier_flags_to_keymod(NSEventModifierFlags flags)
{
	MTY_Mod mod = 0;

	mod |= (flags & NS_MOD_LCTRL) ? MTY_MOD_LCTRL : 0;
	mod |= (flags & NS_MOD_RCTRL) ? MTY_MOD_RCTRL : 0;
	mod |= (flags & NS_MOD_LSHIFT) ? MTY_MOD_LSHIFT : 0;
	mod |= (flags & NS_MOD_RSHIFT) ? MTY_MOD_RSHIFT : 0;
	mod |= (flags & NS_MOD_LCMD) ? MTY_MOD_LWIN : 0;
	mod |= (flags & NS_MOD_RCMD) ? MTY_MOD_RWIN : 0;
	mod |= (flags & NS_MOD_LALT) ? MTY_MOD_LALT : 0;
	mod |= (flags & NS_MOD_RALT) ? MTY_MOD_RALT : 0;
	mod |= (flags & NSEventModifierFlagCapsLock) ? MTY_MOD_CAPS : 0;

	return mod;
}

static MTY_Key window_keycode_to_key(uint16_t kc)
{
	switch (kc) {
		case kVK_ANSI_A:              return MTY_KEY_A; // 0x00
		case kVK_ANSI_S:              return MTY_KEY_S; // 0x01
		case kVK_ANSI_D:              return MTY_KEY_D; // 0x02
		case kVK_ANSI_F:              return MTY_KEY_F; // 0x03
		case kVK_ANSI_H:              return MTY_KEY_H; // 0x04
		case kVK_ANSI_G:              return MTY_KEY_G; // 0x05
		case kVK_ANSI_Z:              return MTY_KEY_Z; // 0x06
		case kVK_ANSI_X:              return MTY_KEY_X; // 0x07
		case kVK_ANSI_C:              return MTY_KEY_C; // 0x08
		case kVK_ANSI_V:              return MTY_KEY_V; // 0x09
		case kVK_ISO_Section:         return MTY_KEY_INTL_BACKSLASH; // 0x0A
		case kVK_ANSI_B:              return MTY_KEY_B; // 0x0B
		case kVK_ANSI_Q:              return MTY_KEY_Q; // 0x0C
		case kVK_ANSI_W:              return MTY_KEY_W; // 0x0D
		case kVK_ANSI_E:              return MTY_KEY_E; // 0x0E
		case kVK_ANSI_R:              return MTY_KEY_R; // 0x0F
		case kVK_ANSI_Y:              return MTY_KEY_Y; // 0x10
		case kVK_ANSI_T:              return MTY_KEY_T; // 0x11
		case kVK_ANSI_1:              return MTY_KEY_1; // 0x12
		case kVK_ANSI_2:              return MTY_KEY_2; // 0x13
		case kVK_ANSI_3:              return MTY_KEY_3; // 0x14
		case kVK_ANSI_4:              return MTY_KEY_4; // 0x15
		case kVK_ANSI_6:              return MTY_KEY_6; // 0x16
		case kVK_ANSI_5:              return MTY_KEY_5; // 0x17
		case kVK_ANSI_Equal:          return MTY_KEY_EQUALS; // 0x18
		case kVK_ANSI_9:              return MTY_KEY_9; // 0x19
		case kVK_ANSI_7:              return MTY_KEY_7; // 0x1A
		case kVK_ANSI_Minus:          return MTY_KEY_MINUS; // 0x1B
		case kVK_ANSI_8:              return MTY_KEY_8; // 0x1C
		case kVK_ANSI_0:              return MTY_KEY_0; // 0x1D
		case kVK_ANSI_RightBracket:   return MTY_KEY_RBRACKET; // 0x1E
		case kVK_ANSI_O:              return MTY_KEY_O; // 0x1F
		case kVK_ANSI_U:              return MTY_KEY_U; // 0x20
		case kVK_ANSI_LeftBracket:    return MTY_KEY_LBRACKET; // 0x21
		case kVK_ANSI_I:              return MTY_KEY_I; // 0x22
		case kVK_ANSI_P:              return MTY_KEY_P; // 0x23
		case kVK_Return:              return MTY_KEY_ENTER; // 0x24
		case kVK_ANSI_L:              return MTY_KEY_L; // 0x25
		case kVK_ANSI_J:              return MTY_KEY_J; // 0x26
		case kVK_ANSI_Quote:          return MTY_KEY_QUOTE; // 0x27
		case kVK_ANSI_K:              return MTY_KEY_K; // 0x28
		case kVK_ANSI_Semicolon:      return MTY_KEY_SEMICOLON; // 0x29
		case kVK_ANSI_Backslash:      return MTY_KEY_BACKSLASH; // 0x2A
		case kVK_ANSI_Comma:          return MTY_KEY_COMMA; // 0x2B
		case kVK_ANSI_Slash:          return MTY_KEY_SLASH; // 0x2C
		case kVK_ANSI_N:              return MTY_KEY_N; // 0x2D
		case kVK_ANSI_M:              return MTY_KEY_M; // 0x2E
		case kVK_ANSI_Period:         return MTY_KEY_PERIOD; // 0x2F
		case kVK_Tab:                 return MTY_KEY_TAB; // 0x30
		case kVK_Space:               return MTY_KEY_SPACE; // 0x31
		case kVK_ANSI_Grave:          return MTY_KEY_GRAVE; // 0x32
		case kVK_Delete:              return MTY_KEY_BACKSPACE; // 0x33
		case 0x34:                    return MTY_KEY_NONE; // 0x34
		case kVK_Escape:              return MTY_KEY_ESCAPE; // 0x35
		case kVK_RightCommand:        return MTY_KEY_RWIN; // 0x36
		case kVK_Command:             return MTY_KEY_LWIN; // 0x37
		case kVK_Shift:               return MTY_KEY_LSHIFT; // 0x38
		case kVK_CapsLock:            return MTY_KEY_CAPS; // 0x39
		case kVK_Option:              return MTY_KEY_LALT; // 0x3A
		case kVK_Control:             return MTY_KEY_LCTRL; // 0x3B
		case kVK_RightShift:          return MTY_KEY_RSHIFT; // 0x3C
		case kVK_RightOption:         return MTY_KEY_RALT; // 0x3D
		case kVK_RightControl:        return MTY_KEY_RCTRL; // 0x3E
		case kVK_Function:            return MTY_KEY_NONE; // 0x3F
		case kVK_F17:                 return MTY_KEY_F17; // 0x40
		case kVK_ANSI_KeypadDecimal:  return MTY_KEY_NP_PERIOD; // 0x41
		case 0x42:                    return MTY_KEY_NONE; // 0x42
		case kVK_ANSI_KeypadMultiply: return MTY_KEY_NP_MULTIPLY; // 0x43
		case 0x44:                    return MTY_KEY_NONE; // 0x44
		case kVK_ANSI_KeypadPlus:     return MTY_KEY_NP_PLUS; // 0x45
		case 0x46:                    return MTY_KEY_NONE; // 0x46
		case kVK_ANSI_KeypadClear:    return MTY_KEY_NUM_LOCK; // 0x47
		case kVK_VolumeUp:            return MTY_KEY_VOLUME_UP; // 0x48
		case kVK_VolumeDown:          return MTY_KEY_VOLUME_DOWN; // 0x49
		case kVK_Mute:                return MTY_KEY_MUTE; // 0x4A
		case kVK_ANSI_KeypadDivide:   return MTY_KEY_NP_DIVIDE; // 0x4B
		case kVK_ANSI_KeypadEnter:    return MTY_KEY_NP_ENTER; // 0x4C
		case kVK_ANSI_KeypadMinus:    return MTY_KEY_NP_MINUS; // 0x4E
		case kVK_F18:                 return MTY_KEY_F18; // 0x4F
		case kVK_F19:                 return MTY_KEY_F19; // 0x50
		case kVK_ANSI_KeypadEquals:   return MTY_KEY_EQUALS; // 0x51
		case kVK_ANSI_Keypad0:        return MTY_KEY_NP_0; // 0x52
		case kVK_ANSI_Keypad1:        return MTY_KEY_NP_1; // 0x53
		case kVK_ANSI_Keypad2:        return MTY_KEY_NP_2; // 0x54
		case kVK_ANSI_Keypad3:        return MTY_KEY_NP_3; // 0x55
		case kVK_ANSI_Keypad4:        return MTY_KEY_NP_4; // 0x56
		case kVK_ANSI_Keypad5:        return MTY_KEY_NP_5; // 0x57
		case kVK_ANSI_Keypad6:        return MTY_KEY_NP_6; // 0x58
		case kVK_ANSI_Keypad7:        return MTY_KEY_NP_7; // 0x59
		case kVK_F20:                 return MTY_KEY_NONE; // 0x5A
		case kVK_ANSI_Keypad8:        return MTY_KEY_NP_8; // 0x5B
		case kVK_ANSI_Keypad9:        return MTY_KEY_NP_9; // 0x5C
		case kVK_JIS_Yen:             return MTY_KEY_YEN; // 0x5D
		case kVK_JIS_Underscore:      return MTY_KEY_RO; // 0x5E
		case kVK_JIS_KeypadComma:     return MTY_KEY_INTL_COMMA; // 0x5F
		case kVK_F5:                  return MTY_KEY_F5; // 0x60
		case kVK_F6:                  return MTY_KEY_F6; // 0x61
		case kVK_F7:                  return MTY_KEY_F7; // 0x62
		case kVK_F3:                  return MTY_KEY_F3; // 0x63
		case kVK_F8:                  return MTY_KEY_F8; // 0x64
		case kVK_F9:                  return MTY_KEY_F9; // 0x65
		case kVK_JIS_Eisu:            return MTY_KEY_MUHENKAN; // 0x66
		case kVK_F11:                 return MTY_KEY_F11; // 0x67
		case kVK_JIS_Kana:            return MTY_KEY_HENKAN; // 0x68
		case kVK_F13:                 return MTY_KEY_PRINT_SCREEN; // 0x69
		case kVK_F16:                 return MTY_KEY_F16; // 0x6A
		case kVK_F14:                 return MTY_KEY_SCROLL_LOCK; // 0x6B
		case kVK_F10:                 return MTY_KEY_F10; // 0x6D
		case kVK_WinApp:              return MTY_KEY_APP; // 0x6E
		case kVK_F12:                 return MTY_KEY_F12; // 0x6F
		case 0x70:                    return MTY_KEY_NONE; // 0x70
		case kVK_F15:                 return MTY_KEY_PAUSE; // 0x71
		case kVK_Help:                return MTY_KEY_INSERT; // 0x72
		case kVK_Home:                return MTY_KEY_HOME; // 0x73
		case kVK_PageUp:              return MTY_KEY_PAGE_UP; // 0x74
		case kVK_ForwardDelete:       return MTY_KEY_DELETE; // 0x75
		case kVK_F4:                  return MTY_KEY_F4; // 0x76
		case kVK_End:                 return MTY_KEY_END; // 0x77
		case kVK_F2:                  return MTY_KEY_F2; // 0x78
		case kVK_PageDown:            return MTY_KEY_PAGE_DOWN; // 0x79
		case kVK_F1:                  return MTY_KEY_F1; // 0x7A
		case kVK_LeftArrow:           return MTY_KEY_LEFT; // 0x7B
		case kVK_RightArrow:          return MTY_KEY_RIGHT; // 0x7C
		case kVK_DownArrow:           return MTY_KEY_DOWN; // 0x7D
		case kVK_UpArrow:             return MTY_KEY_UP; // 0x7E
	}

	return MTY_KEY_NONE;
}

static const char *window_keycode_to_text(uint16_t kc)
{
	switch (kc) {
		case kVK_Return:              return "Enter"; // 0x24
		case kVK_Tab:                 return "Tab"; // 0x30
		case kVK_Space:               return "Space"; // 0x31
		case kVK_Delete:              return "Backspace"; // 0x33
		case kVK_Escape:              return "Esc"; // 0x35
		case kVK_RightCommand:        return "Command"; // 0x36
		case kVK_Command:             return "Command"; // 0x37
		case kVK_Shift:               return "Shift"; // 0x38
		case kVK_CapsLock:            return "Caps"; // 0x39
		case kVK_Option:              return "Alt"; // 0x3A
		case kVK_Control:             return "Alt"; // 0x3B
		case kVK_RightShift:          return "Shift"; // 0x3C
		case kVK_RightOption:         return "Alt"; // 0x3D
		case kVK_RightControl:        return "Ctrl"; // 0x3E
		case kVK_VolumeUp:            return "Volume Up"; // 0x48
		case kVK_VolumeDown:          return "Volume Down"; // 0x49
		case kVK_Mute:                return "Mute"; // 0x4A
		case kVK_F18:                 return "F18"; // 0x4F
		case kVK_F19:                 return "F19"; // 0x50
		case kVK_F20:                 return "F20"; // 0x5A
		case kVK_F5:                  return "F5"; // 0x60
		case kVK_F6:                  return "F6"; // 0x61
		case kVK_F7:                  return "F7"; // 0x62
		case kVK_F3:                  return "F3"; // 0x63
		case kVK_F8:                  return "F8"; // 0x64
		case kVK_F9:                  return "F9"; // 0x65
		case kVK_F11:                 return "F11"; // 0x67
		case kVK_F13:                 return "Print Screen"; // 0x69
		case kVK_F16:                 return "F16"; // 0x6A
		case kVK_F14:                 return "F14"; // 0x6B
		case kVK_F10:                 return "F10"; // 0x6D
		case kVK_WinApp:              return "App"; // 0x6E
		case kVK_F12:                 return "F12"; // 0x6F
		case kVK_F15:                 return "F15"; // 0x71
		case kVK_Help:                return "Insert"; // 0x72
		case kVK_Home:                return "Home"; // 0x73
		case kVK_PageUp:              return "Page Up"; // 0x74
		case kVK_ForwardDelete:       return "Delete"; // 0x75
		case kVK_F4:                  return "F4"; // 0x76
		case kVK_End:                 return "End"; // 0x77
		case kVK_F2:                  return "F2"; // 0x78
		case kVK_PageDown:            return "Page Down"; // 0x79
		case kVK_F1:                  return "F1"; // 0x7A
		case kVK_LeftArrow:           return "Left"; // 0x7B
		case kVK_RightArrow:          return "Right"; // 0x7C
		case kVK_DownArrow:           return "Down"; // 0x7D
		case kVK_UpArrow:             return "Up"; // 0x7E
	}

	return NULL;
}
