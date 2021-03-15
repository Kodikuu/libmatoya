// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include "app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "web.h"
#include "keymap.h"

struct MTY_App {
	MTY_Hash *hotkey;
	MTY_EventFunc event_func;
	MTY_AppFunc app_func;
	MTY_Detach detach;
	MTY_ControllerEvent cevt[4];
	void *opaque;

	MTY_GFX api;
	bool kb_grab;
	struct gfx_ctx *gfx_ctx;
};

static void __attribute__((constructor)) app_global_init(void)
{
	web_set_mem_funcs(MTY_Alloc, MTY_Free);

	// WASI will buffer stdout and stderr by default, disable it
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
}


// Events

static void window_mouse_motion(MTY_App *ctx, bool relative, int32_t x, int32_t y)
{
	MTY_Event evt = {0};
	evt.type = MTY_EVENT_MOTION;
	evt.motion.relative = relative;
	evt.motion.x = x;
	evt.motion.y = y;

	ctx->event_func(&evt, ctx->opaque);
}

static void window_mouse_button(MTY_App *ctx, bool pressed, int32_t button, int32_t x, int32_t y)
{
	MTY_Event evt = {0};
	evt.type = MTY_EVENT_BUTTON;
	evt.button.pressed = pressed;
	evt.button.button =
		button == 0 ? MTY_BUTTON_LEFT :
		button == 1 ? MTY_BUTTON_MIDDLE :
		button == 2 ? MTY_BUTTON_RIGHT :
		button == 3 ? MTY_BUTTON_X1 :
		button == 4 ? MTY_BUTTON_X2 :
		MTY_BUTTON_NONE;

	evt.button.x = x;
	evt.button.y = y;

	ctx->event_func(&evt, ctx->opaque);
}

static void window_mouse_scroll(MTY_App *ctx, int32_t x, int32_t y)
{
	MTY_Event evt = {0};
	evt.type = MTY_EVENT_SCROLL;
	evt.scroll.x = x;
	evt.scroll.y = -y;

	ctx->event_func(&evt, ctx->opaque);
}

static void app_kb_to_hotkey(MTY_App *app, MTY_Event *evt)
{
	MTY_Mod mod = evt->key.mod & 0xFF;
	uint32_t hotkey = (uint32_t) (uintptr_t) MTY_HashGetInt(app->hotkey, (mod << 16) | evt->key.key);

	if (hotkey != 0) {
		if (evt->key.pressed) {
			evt->type = MTY_EVENT_HOTKEY;
			evt->hotkey = hotkey;

		} else {
			evt->type = MTY_EVENT_NONE;
		}
	}
}

static bool window_allow_default(MTY_Mod mod, MTY_Key key)
{
	// The "allowed" browser hotkey list. Copy/Paste, Refresh, fullscreen, developer console, and tab switching

	return
		(((mod & MTY_MOD_CTRL) || (mod & MTY_MOD_WIN)) && key == MTY_KEY_V) ||
		(((mod & MTY_MOD_CTRL) || (mod & MTY_MOD_WIN)) && key == MTY_KEY_C) ||
		((mod & MTY_MOD_CTRL) && (mod & MTY_MOD_SHIFT) && key == MTY_KEY_I) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_R) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_F5) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_1) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_2) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_3) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_4) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_5) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_6) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_7) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_8) ||
		((mod & MTY_MOD_CTRL) && key == MTY_KEY_9) ||
		(key == MTY_KEY_F5) ||
		(key == MTY_KEY_F11) ||
		(key == MTY_KEY_F12);
}

static bool window_keyboard(MTY_App *ctx, bool pressed, MTY_Key key, const char *text, uint32_t mods)
{
	MTY_Event evt = {0};

	if (text) {
		evt.type = MTY_EVENT_TEXT;
		snprintf(evt.text, 8, "%s", text);

		ctx->event_func(&evt, ctx->opaque);
	}

	if (key != 0) {
		evt.type = MTY_EVENT_KEYBOARD;
		evt.key.key = key;
		evt.key.pressed = pressed;

		if (mods & 0x01) evt.key.mod |= MTY_MOD_LSHIFT;
		if (mods & 0x02) evt.key.mod |= MTY_MOD_LCTRL;
		if (mods & 0x04) evt.key.mod |= MTY_MOD_LALT;
		if (mods & 0x08) evt.key.mod |= MTY_MOD_LWIN;
		if (mods & 0x10) evt.key.mod |= MTY_MOD_CAPS;
		if (mods & 0x20) evt.key.mod |= MTY_MOD_NUM;

		app_kb_to_hotkey(ctx, &evt);
		ctx->event_func(&evt, ctx->opaque);

		return !window_allow_default(evt.key.mod, evt.key.key) || ctx->kb_grab;
	}

	return true;
}

static void window_focus(MTY_App *ctx, bool focus)
{
	MTY_Event evt = {0};
	evt.type = MTY_EVENT_FOCUS;
	evt.focus = focus;

	ctx->event_func(&evt, ctx->opaque);
}

static void window_drop(MTY_App *ctx, const char *name, const void *data, size_t size)
{
	MTY_Event evt = {0};
	evt.type = MTY_EVENT_DROP;
	evt.drop.name = name;
	evt.drop.data = data;
	evt.drop.size = size;

	ctx->event_func(&evt, ctx->opaque);
}

static void window_clean_value(int16_t *value)
{
	// Dead zone
	if (abs(*value) < 2000)
		*value = 0;

	// Reduce precision
	if (*value != INT16_MIN && *value != INT16_MAX)
		*value &= 0xFFFE;
}

static void window_controller(MTY_App *ctx, uint32_t id, uint32_t state, uint32_t buttons,
	float lx, float ly, float rx, float ry, float lt, float rt)
{
	#define TESTB(i) \
		((buttons & (i)) == (i))

	MTY_Event evt = {0};
	evt.type = MTY_EVENT_CONTROLLER;

	MTY_ControllerEvent *c = &evt.controller;
	MTY_ControllerEvent *prev = &ctx->cevt[id];
	c->type = MTY_CTYPE_DEFAULT;
	c->numButtons = 16;
	c->numValues = 7;
	c->vid = 0xCDD;
	c->pid = 0xCDD;
	c->id = id;

	c->buttons[MTY_CBUTTON_A] = TESTB(0x0001);
	c->buttons[MTY_CBUTTON_B] = TESTB(0x0002);
	c->buttons[MTY_CBUTTON_X] = TESTB(0x0004);
	c->buttons[MTY_CBUTTON_Y] = TESTB(0x0008);
	c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = TESTB(0x0010);
	c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = TESTB(0x0020);
	c->buttons[MTY_CBUTTON_BACK] = TESTB(0x0100);
	c->buttons[MTY_CBUTTON_START] = TESTB(0x0200);
	c->buttons[MTY_CBUTTON_LEFT_THUMB] = TESTB(0x0400);
	c->buttons[MTY_CBUTTON_RIGHT_THUMB] = TESTB(0x0800);

	c->values[MTY_CVALUE_THUMB_LX].data = lx < 0.0f ? lrint(lx * abs(INT16_MIN)) : lrint(lx * INT16_MAX);
	c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
	c->values[MTY_CVALUE_THUMB_LX].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_LX].max = INT16_MAX;

	c->values[MTY_CVALUE_THUMB_LY].data = ly > 0.0f ? lrint(-ly * abs(INT16_MIN)) : lrint(-ly * INT16_MAX);
	c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
	c->values[MTY_CVALUE_THUMB_LY].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_LY].max = INT16_MAX;

	c->values[MTY_CVALUE_THUMB_RX].data = rx < 0.0f ? lrint(rx * abs(INT16_MIN)) : lrint(rx * INT16_MAX);
	c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
	c->values[MTY_CVALUE_THUMB_RX].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_RX].max = INT16_MAX;

	c->values[MTY_CVALUE_THUMB_RY].data = ry > 0.0f ? lrint(-ry * abs(INT16_MIN)) : lrint(-ry * INT16_MAX);
	c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
	c->values[MTY_CVALUE_THUMB_RY].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_RY].max = INT16_MAX;

	c->values[MTY_CVALUE_TRIGGER_L].data = lrint(lt * UINT8_MAX);
	c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
	c->values[MTY_CVALUE_TRIGGER_L].min = 0;
	c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

	c->values[MTY_CVALUE_TRIGGER_R].data = lrint(rt * UINT8_MAX);
	c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
	c->values[MTY_CVALUE_TRIGGER_R].min = 0;
	c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;

	c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = c->values[MTY_CVALUE_TRIGGER_L].data > 0;
	c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = c->values[MTY_CVALUE_TRIGGER_R].data > 0;

	bool up = TESTB(0x1000);
	bool down = TESTB(0x2000);
	bool left = TESTB(0x4000);
	bool right = TESTB(0x8000);

	c->values[MTY_CVALUE_DPAD].data = (up && right) ? 1 : (right && down) ? 3 :
		(down && left) ? 5 : (left && up) ? 7 : up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;
	c->values[MTY_CVALUE_DPAD].usage = 0x39;
	c->values[MTY_CVALUE_DPAD].min = 0;
	c->values[MTY_CVALUE_DPAD].max = 7;

	// Connect
	if (state == 1) {
		MTY_Event cevt = evt;
		cevt.type = MTY_EVENT_CONNECT;
		ctx->event_func(&cevt, ctx->opaque);

	// Disconnect
	} else if (state == 2) {
		evt.type = MTY_EVENT_DISCONNECT;
	}

	// Axis dead zone, precision reduction -- helps with deduplication
	window_clean_value(&c->values[MTY_CVALUE_THUMB_LX].data);
	window_clean_value(&c->values[MTY_CVALUE_THUMB_LY].data);
	window_clean_value(&c->values[MTY_CVALUE_THUMB_RX].data);
	window_clean_value(&c->values[MTY_CVALUE_THUMB_RY].data);

	// Deduplication
	bool button_diff = memcmp(c->buttons, prev->buttons, c->numButtons * sizeof(bool));
	bool values_diff = memcmp(c->values, prev->values, c->numValues * sizeof(MTY_Value));

	if (button_diff || values_diff || evt.type != MTY_EVENT_CONTROLLER)
		ctx->event_func(&evt, ctx->opaque);

	*prev = evt.controller;
}


// App / Window

void MTY_HotkeyToString(MTY_Mod mod, MTY_Key key, char *str, size_t len)
{
	memset(str, 0, len);

	MTY_Strcat(str, len, (mod & MTY_MOD_WIN) ? "Super+" : "");
	MTY_Strcat(str, len, (mod & MTY_MOD_CTRL) ? "Ctrl+" : "");
	MTY_Strcat(str, len, (mod & MTY_MOD_ALT) ? "Alt+" : "");
	MTY_Strcat(str, len, (mod & MTY_MOD_SHIFT) ? "Shift+" : "");

	char key_str[32];
	if (web_get_key(key, key_str, 32))
		MTY_Strcat(str, len, key_str);
}

uint32_t MTY_AppGetHotkey(MTY_App *ctx, MTY_Scope scope, MTY_Mod mod, MTY_Key key)
{
	mod &= 0xFF;

	return (uint32_t) (uintptr_t) MTY_HashGetInt(ctx->hotkey, (mod << 16) | key);
}

void MTY_AppSetHotkey(MTY_App *ctx, MTY_Scope scope, MTY_Mod mod, MTY_Key key, uint32_t id)
{
	mod &= 0xFF;
	MTY_HashSetInt(ctx->hotkey, (mod << 16) | key, (void *) (uintptr_t) id);
}

void MTY_AppRemoveHotkeys(MTY_App *ctx, MTY_Scope scope)
{
	MTY_HashDestroy(&ctx->hotkey, NULL);
	ctx->hotkey = MTY_HashCreate(0);
}

char *MTY_AppGetClipboard(MTY_App *app)
{
	return web_get_clipboard();
}

void MTY_AppSetClipboard(MTY_App *app, const char *text)
{
	web_set_clipboard(text);
}

void MTY_AppSetPNGCursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
	web_set_png_cursor(image, size, hotX, hotY);
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
	web_use_default_cursor(useDefault);
}

void MTY_AppShowCursor(MTY_App *ctx, bool show)
{
	web_show_cursor(show);
}

bool MTY_AppCanWarpCursor(MTY_App *ctx)
{
	return false;
}

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_EventFunc eventFunc, void *opaque)
{
	MTY_App *ctx = MTY_Alloc(1, sizeof(MTY_App));
	ctx->app_func = appFunc;
	ctx->event_func = eventFunc;
	ctx->opaque = opaque;

	web_attach_events(ctx, window_mouse_motion, window_mouse_button,
		window_mouse_scroll, window_keyboard, window_focus, window_drop);

	ctx->hotkey = MTY_HashCreate(0);

	app_set_keys();

	return ctx;
}

void MTY_AppDestroy(MTY_App **app)
{
	if (!app || !*app)
		return;

	MTY_App *ctx = *app;

	MTY_HashDestroy(&ctx->hotkey, NULL);

	MTY_Free(ctx);
	*app = NULL;
}

void MTY_AppRun(MTY_App *ctx)
{
	web_raf(ctx, ctx->app_func, window_controller, ctx->opaque);
}

void MTY_AppEnableScreenSaver(MTY_App *app, bool enable)
{
	web_wake_lock(enable);
}

bool MTY_AppKeyboardIsGrabbed(MTY_App *ctx)
{
	return ctx->kb_grab;
}

void MTY_AppGrabKeyboard(MTY_App *app, bool grab)
{
	app->kb_grab = grab;
}

void MTY_AppSetRelativeMouse(MTY_App *app, bool relative)
{
	web_set_pointer_lock(relative);
}

bool MTY_AppGetRelativeMouse(MTY_App *app)
{
	return web_get_relative();
}

bool MTY_AppIsActive(MTY_App *ctx)
{
	return web_has_focus();
}

void MTY_AppControllerRumble(MTY_App *app, uint32_t id, uint16_t low, uint16_t high)
{
	web_rumble_gamepad(id, (float) low / (float) UINT16_MAX, (float) high / (float) UINT16_MAX);
}

MTY_Window MTY_WindowCreate(MTY_App *app, const char *title, const MTY_WindowDesc *desc)
{
	MTY_WindowSetTitle(app, 0, title);

	return 0;
}

void MTY_WindowSetTitle(MTY_App *app, MTY_Window window, const char *title)
{
	web_set_title(title);
}

bool MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	web_get_size(width, height);

	return true;
}

bool MTY_WindowGetScreenSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	web_get_screen_size(width, height);

	return true;
}

float MTY_WindowGetScale(MTY_App *app, MTY_Window window)
{
	return web_get_pixel_ratio();
}

bool MTY_WindowIsVisible(MTY_App *app, MTY_Window window)
{
	return web_is_visible();
}

bool MTY_WindowIsActive(MTY_App *app, MTY_Window window)
{
	return web_has_focus();
}

void MTY_WindowSetFullscreen(MTY_App *app, MTY_Window window, bool fullscreen)
{
	web_set_fullscreen(fullscreen);
}

bool MTY_WindowIsFullscreen(MTY_App *app, MTY_Window window)
{
	return web_get_fullscreen();
}



// Window Private

void mty_window_set_gfx(MTY_App *app, MTY_Window window, MTY_GFX api, struct gfx_ctx *gfx_ctx)
{
	app->api = api;
	app->gfx_ctx = gfx_ctx;
}

MTY_GFX mty_window_get_gfx(MTY_App *app, MTY_Window window, struct gfx_ctx **gfx_ctx)
{
	if (gfx_ctx)
		*gfx_ctx = app->gfx_ctx;

	return app->api;
}

void *mty_window_get_native(MTY_App *app, MTY_Window window)
{
	return (void *) (uintptr_t) 0xCDD;
}


// Misc

void MTY_MessageBox(const char *title, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char *msg = MTY_VsprintfD(fmt, args);

	va_end(args);

	web_alert(title, msg);
	MTY_Free(msg);
}

void MTY_SetAppID(const char *id)
{
}


// Unimplemented

void MTY_AppDetach(MTY_App *app, MTY_Detach type)
{
	app->detach = type;
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
	return app->detach;
}

void MTY_AppActivate(MTY_App *app, bool active)
{
}

void MTY_WindowActivate(MTY_App *app, MTY_Window window, bool active)
{
}

void MTY_WindowWarpCursor(MTY_App *app, MTY_Window window, uint32_t x, uint32_t y)
{
}

bool MTY_AppMouseIsGrabbed(MTY_App *ctx)
{
	return false;
}

void MTY_AppGrabMouse(MTY_App *app, bool grab)
{
}

void MTY_AppSetTimeout(MTY_App *ctx, uint32_t timeout)
{
}

void MTY_WindowDestroy(MTY_App *app, MTY_Window window)
{
}

bool MTY_WindowExists(MTY_App *app, MTY_Window window)
{
	return true;
}

void *MTY_GLGetProcAddress(const char *name)
{
	return NULL;
}

void MTY_AppSetTray(MTY_App *app, const char *tooltip, const MTY_MenuItem *items, uint32_t len)
{
}

void MTY_AppRemoveTray(MTY_App *app)
{
}

void MTY_AppNotification(MTY_App *app, const char *title, const char *msg)
{
}

void MTY_AppShowSoftKeyboard(MTY_App *app, bool show)
{
}

bool MTY_AppSoftKeyboardIsShowing(MTY_App *app)
{
	return false;
}

MTY_Orientation MTY_AppGetOrientation(MTY_App *ctx)
{
	return MTY_ORIENTATION_USER;
}

void MTY_AppSetOrientation(MTY_App *app, MTY_Orientation orientation)
{
}

void MTY_AppEnableGlobalHotkeys(MTY_App *app, bool enable)
{
}

bool MTY_AppPenIsEnabled(MTY_App *ctx)
{
	return false;
}

void MTY_AppEnablePen(MTY_App *ctx, bool enable)
{
}

MTY_GFXState MTY_WindowGFXState(MTY_App *app, MTY_Window window)
{
	return MTY_GFX_STATE_NORMAL;
}

void MTY_AppSetInputMode(MTY_App *ctx, MTY_Input mode)
{
}
