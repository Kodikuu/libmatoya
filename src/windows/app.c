// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include "app.h"

#include <stdio.h>
#include <math.h>

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <shellscalingapi.h>

#include "wsize.h"
#include "hid/driver.h"

#define WINDOW_CLASS_NAME L"MTY_Window"
#define WINDOW_RI_MAX     (32 * 1024)

struct window {
	MTY_App *app;
	MTY_Window window;
	MTY_GFX api;
	HWND hwnd;
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
	uint32_t min_width;
	uint32_t min_height;
	struct gfx_ctx *gfx_ctx;
	RAWINPUT *ri;
};

struct MTY_App {
	MTY_AppFunc app_func;
	MTY_EventFunc event_func;
	void *opaque;

	WNDCLASSEX wc;
	ATOM class;
	UINT tb_msg;
	RECT clip;
	HICON cursor;
	HICON custom_cursor;
	HINSTANCE instance;
	HHOOK kbhook;
	DWORD cb_seq;
	bool pen_in_range;
	bool pen_enabled;
	bool touch_active;
	bool relative;
	bool kbgrab;
	bool mgrab;
	bool default_cursor;
	bool hide_cursor;
	bool ghk_disabled;
	bool filter_move;
	uint64_t prev_state;
	uint64_t state;
	uint32_t timeout;
	int32_t last_x;
	int32_t last_y;
	struct hid *hid;
	MTY_Button buttons;
	MTY_Detach detach;
	MTY_Hash *hotkey;
	MTY_Hash *ghotkey;

	struct window *windows[MTY_WINDOW_MAX];

	struct {
		HMENU menu;
		NOTIFYICONDATA nid;
		MTY_Time ts;
		MTY_Time rctimer;
		bool init;
		bool want;

		struct menu_item {
			WCHAR *label;
			uint32_t trayID;
			MTY_MenuItemCheckedFunc checked;
		} *items;

		uint32_t len;
	} tray;
};


// MTY -> VK mouse map

static const int APP_MOUSE_MAP[] = {
	[MTY_BUTTON_LEFT]   = VK_LBUTTON,
	[MTY_BUTTON_RIGHT]  = VK_RBUTTON,
	[MTY_BUTTON_MIDDLE] = VK_MBUTTON,
	[MTY_BUTTON_X1]     = VK_XBUTTON1,
	[MTY_BUTTON_X2]     = VK_XBUTTON2,
};

#define APP_MOUSE_MAX (sizeof(APP_MOUSE_MAP) / sizeof(int))


// Low level keyboard hook needs a global HWND

static HWND WINDOW_KB_HWND;


// Weak Linking

static HRESULT (WINAPI *_GetDpiForMonitor)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY);
static BOOL (WINAPI *_GetPointerType)(UINT32 pointerId, POINTER_INPUT_TYPE *pointerType);
static BOOL (WINAPI *_GetPointerPenInfo)(UINT32 pointerId, POINTER_PEN_INFO *penInfo);


// App Static Helpers

static HWND app_get_main_hwnd(MTY_App *app)
{
	if (!app->windows[0])
		return NULL;

	return app->windows[0]->hwnd;
}

static struct window *app_get_main_window(MTY_App *app)
{
	return app->windows[0];
}

static struct window *app_get_window(MTY_App *app, MTY_Window window)
{
	return window < 0 ? NULL : app->windows[window];
}

static struct window *app_get_focus_window(MTY_App *app)
{
	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++)
		if (MTY_WindowIsActive(app, x))
			return app_get_window(app, x);

	return NULL;
}

static MTY_Window app_find_open_window(MTY_App *app)
{
	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++)
		if (!app->windows[x])
			return x;

	return -1;
}

static bool app_hwnd_visible(HWND hwnd)
{
	return IsWindowVisible(hwnd) && !IsIconic(hwnd);
}

static void app_hwnd_activate(HWND hwnd, bool active)
{
	if (active) {
		if (!app_hwnd_visible(hwnd))
			ShowWindow(hwnd, SW_RESTORE);

		SetForegroundWindow(hwnd);

	} else {
		ShowWindow(hwnd, SW_HIDE);
	}
}

static float app_hwnd_get_scale(HWND hwnd)
{
	if (_GetDpiForMonitor) {
		HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

		if (mon) {
			UINT x = 0;
			UINT y = 0;

			if (_GetDpiForMonitor(mon, MDT_EFFECTIVE_DPI, &x, &y) == S_OK)
				return (float) x / 96.0f;
		}
	}

	return 1.0f;
}

static bool app_hwnd_active(HWND hwnd)
{
	return GetForegroundWindow() == hwnd && app_hwnd_visible(hwnd);
}

static void app_register_raw_input(USHORT usage_page, USHORT usage, DWORD flags, HWND hwnd)
{
	RAWINPUTDEVICE rid = {0};
	rid.usUsagePage = usage_page;
	rid.usUsage = usage;
	rid.dwFlags = flags;
	rid.hwndTarget = hwnd;
	if (!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
		MTY_Log("'RegisterRawInputDevices' failed with error 0x%X", GetLastError());
}


// Hotkeys

static void app_unregister_global_hotkeys(MTY_App *app)
{
	HWND hwnd = app_get_main_hwnd(app);
	if (!hwnd)
		return;

	uint64_t i = 0;
	int64_t key = 0;

	while (MTY_HashNextKeyInt(app->ghotkey, &i, &key))
		if (key > 0)
			if (!UnregisterHotKey(hwnd, (int32_t) key))
				MTY_Log("'UnregisterHotKey' failed with error 0x%X", GetLastError());
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

static bool app_key_to_str(MTY_Key key, char *str, size_t len)
{
	LONG lparam = (key & 0xFF) << 16;
	if (key & 0x100)
		lparam |= 1 << 24;

	wchar_t wstr[8] = {0};
	if (GetKeyNameText(lparam, wstr, 8))
		return MTY_WideToMulti(wstr, str, len);

	return false;
}

void MTY_HotkeyToString(MTY_Mod mod, MTY_Key key, char *str, size_t len)
{
	memset(str, 0, len);

	MTY_Strcat(str, len, (mod & MTY_MOD_WIN) ? "Win+" : "");
	MTY_Strcat(str, len, (mod & MTY_MOD_CTRL) ? "Ctrl+" : "");
	MTY_Strcat(str, len, (mod & MTY_MOD_ALT) ? "Alt+" : "");
	MTY_Strcat(str, len, (mod & MTY_MOD_SHIFT) ? "Shift+" : "");

	if (key != MTY_KEY_NONE) {
		char c[8] = {0};

		if (app_key_to_str(key, c, 8))
			MTY_Strcat(str, len, c);
	}
}

void MTY_AppEnableGlobalHotkeys(MTY_App *app, bool enable)
{
	app->ghk_disabled = !enable;
}

void MTY_AppSetHotkey(MTY_App *app, MTY_Scope scope, MTY_Mod mod, MTY_Key key, uint32_t id)
{
	mod &= 0xFF;

	if (scope == MTY_SCOPE_LOCAL) {
		MTY_HashSetInt(app->hotkey, (mod << 16) | key, (void *) (uintptr_t) id);

	} else {
		HWND hwnd = app_get_main_hwnd(app);
		if (hwnd) {
			UINT wmod = MOD_NOREPEAT |
				((mod & MTY_MOD_ALT) ? MOD_ALT : 0) |
				((mod & MTY_MOD_CTRL) ? MOD_CONTROL : 0) |
				((mod & MTY_MOD_SHIFT) ? MOD_SHIFT : 0) |
				((mod & MTY_MOD_WIN) ? MOD_SHIFT : 0);

			UINT vk = MapVirtualKey(key & 0xFF, MAPVK_VSC_TO_VK);

			if (vk > 0) {
				uint32_t lookup = (wmod << 8) | vk;

				if (MTY_HashGetInt(app->ghotkey, lookup) || id == 0)
					if (!UnregisterHotKey(hwnd, lookup))
						MTY_Log("'UnregisterHotKey' failed with error 0x%X", GetLastError());

				if (id > 0)
					if (!RegisterHotKey(hwnd, lookup, wmod, vk))
						MTY_Log("'RegisterHotKey' failed with error 0x%X", GetLastError());

				MTY_HashSetInt(app->ghotkey, lookup, (void *) (uintptr_t) id);
			}
		}
	}
}

void MTY_AppRemoveHotkeys(MTY_App *app, MTY_Scope scope)
{
	if (scope == MTY_SCOPE_GLOBAL) {
		app_unregister_global_hotkeys(app);

		MTY_HashDestroy(&app->ghotkey, NULL);
		app->ghotkey = MTY_HashCreate(0);

	} else if (scope == MTY_SCOPE_LOCAL) {
		MTY_HashDestroy(&app->hotkey, NULL);
		app->hotkey = MTY_HashCreate(0);
	}
}


// App Tray

#define APP_TRAY_CALLBACK (WM_USER + 510)
#define APP_TRAY_UID      1337
#define APP_TRAY_FIRST    1000

static HMENU app_tray_menu(struct menu_item *items, uint32_t len, void *opaque)
{
	HMENU menu = CreatePopupMenu();

	for (uint32_t x = 0; x < len; x++) {
		if (items[x].label[0]) {
			MENUITEMINFO item = {0};
			item.cbSize = sizeof(MENUITEMINFO);
			item.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
			item.wID = APP_TRAY_FIRST + x;
			item.dwTypeData = items[x].label;
			item.dwItemData = (ULONG_PTR) items[x].trayID;

			if (items[x].checked && items[x].checked(opaque))
				item.fState |= MFS_CHECKED;

			InsertMenuItem(menu, APP_TRAY_FIRST + x, TRUE, &item);

		} else {
			InsertMenu(menu, APP_TRAY_FIRST + x, MF_SEPARATOR, TRUE, L"");
		}
	}

	return menu;
}

static bool app_tray_get_location(HWND hwnd, uint32_t *x, uint32_t *y)
{
	NOTIFYICONIDENTIFIER id = {0};
	id.cbSize = sizeof(NOTIFYICONIDENTIFIER);
	id.hWnd = hwnd;
	id.uID = APP_TRAY_UID;
	id.guidItem = GUID_NULL;

	RECT r = {0};
	if (Shell_NotifyIconGetRect(&id, &r) == S_OK) {
		if (x)
			*x = r.left + (r.right - r.left) / 2;

		if (y)
			*y = r.top + (r.bottom - r.top) / 2;

		return true;
	}

	return false;
}

static void app_tray_set_nid(NOTIFYICONDATA *nid, UINT uflags, HWND hwnd, HICON smicon, HICON lgicon)
{
	nid->cbSize = sizeof(NOTIFYICONDATA);
	nid->hWnd = hwnd;
	nid->uID = APP_TRAY_UID;
	nid->uFlags = NIF_MESSAGE | uflags;
	nid->uCallbackMessage = APP_TRAY_CALLBACK;
	nid->hIcon = smicon;
	nid->hBalloonIcon = lgicon;
	nid->dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON;

	memset(nid->szInfoTitle, 0, sizeof(nid->szInfoTitle));
	memset(nid->szInfo, 0, sizeof(nid->szInfo));
}

static void app_tray_recreate(MTY_App *app, struct window *ctx)
{
	app_tray_set_nid(&app->tray.nid, NIF_TIP | NIF_ICON, ctx->hwnd, app->wc.hIconSm, app->wc.hIcon);

	if (app->tray.init)
		Shell_NotifyIcon(NIM_DELETE, &app->tray.nid);

	app->tray.init = Shell_NotifyIcon(NIM_ADD, &app->tray.nid);
}

static void app_tray_retry(MTY_App *app, struct window *ctx)
{
	if (!app->tray.init && app->tray.want) {
		MTY_Time now = MTY_GetTime();

		if (MTY_TimeDiff(app->tray.rctimer, now) > 2000.0f) {
			app_tray_recreate(app, ctx);
			app->tray.rctimer = now;
		}
	}
}

static void app_tray_msg(MTY_App *app, UINT msg, WPARAM wparam, LPARAM lparam, MTY_Event *evt)
{
	struct window *ctx = app_get_main_window(app);
	if (!ctx)
		return;

	// Tray interaction
	if (msg == APP_TRAY_CALLBACK) {
		switch (lparam) {
			case NIN_BALLOONUSERCLICK:
				MTY_AppActivate(app, true);
				break;
			case WM_LBUTTONUP:
				MTY_Time now = MTY_GetTime();

				if (MTY_TimeDiff(app->tray.ts, now) > GetDoubleClickTime() * 2) {
					MTY_AppActivate(app, true);
					app->tray.ts = now;
				}
				break;
			case WM_RBUTTONUP:
				if (app->tray.menu)
					DestroyMenu(app->tray.menu);

				app->tray.menu = app_tray_menu(app->tray.items, app->tray.len, app->opaque);

				if (app->tray.menu) {
					SetForegroundWindow(ctx->hwnd);
					SendMessage(ctx->hwnd, WM_INITMENUPOPUP, (WPARAM) app->tray.menu, 0);

					uint32_t x = 0;
					uint32_t y = 0;
					if (app_tray_get_location(ctx->hwnd, &x, &y)) {
						WORD cmd = (WORD) TrackPopupMenu(app->tray.menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON |
							TPM_RETURNCMD | TPM_NONOTIFY, x, y, 0, ctx->hwnd, NULL);

						SendMessage(ctx->hwnd, WM_COMMAND, cmd, 0);
						PostMessage(ctx->hwnd, WM_NULL, 0, 0);
					}
				}
				break;
		}

	// Recreation on taskbar recreation (DPI changes)
	} else if (msg == app->tb_msg) {
		if (app->tray.init)
			app_tray_recreate(app, ctx);

	// Menu command
	} else if (msg == WM_COMMAND && wparam >= APP_TRAY_FIRST && app->tray.menu) {
		MENUITEMINFO item = {0};
		item.cbSize = sizeof(MENUITEMINFO);
		item.fMask = MIIM_ID | MIIM_DATA;

		if (GetMenuItemInfo(app->tray.menu, (UINT) wparam, FALSE, &item)) {
			evt->type = MTY_EVENT_TRAY;
			evt->trayID = (uint32_t) item.dwItemData;
		}
	}
}

void MTY_AppSetTray(MTY_App *app, const char *tooltip, const MTY_MenuItem *items, uint32_t len)
{
	MTY_AppRemoveTray(app);

	app->tray.want = true;
	app->tray.rctimer = MTY_GetTime();
	app->tray.items = MTY_Alloc(len, sizeof(struct menu_item));
	app->tray.len = len;

	for (uint32_t x = 0; x < len; x++) {
		app->tray.items[x].label = MTY_MultiToWideD(items[x].label);
		app->tray.items[x].checked = items[x].checked;
		app->tray.items[x].trayID = items[x].trayID;
	}

	HWND hwnd = app_get_main_hwnd(app);
	if (hwnd) {
		app_tray_set_nid(&app->tray.nid, NIF_TIP | NIF_ICON, hwnd, app->wc.hIconSm, app->wc.hIcon);
		MTY_MultiToWide(tooltip, app->tray.nid.szTip, sizeof(app->tray.nid.szTip) / sizeof(wchar_t));

		app->tray.init = Shell_NotifyIcon(NIM_ADD, &app->tray.nid);
		if (!app->tray.init)
			MTY_Log("'Shell_NotifyIcon' failed");
	}
}

void MTY_AppRemoveTray(MTY_App *app)
{
	for (uint32_t x = 0; x < app->tray.len; x++)
		MTY_Free(app->tray.items[x].label);

	MTY_Free(app->tray.items);

	app->tray.items = NULL;
	app->tray.want = false;
	app->tray.len = 0;

	HWND hwnd = app_get_main_hwnd(app);
	if (hwnd) {
		app_tray_set_nid(&app->tray.nid, NIF_TIP | NIF_ICON, hwnd, app->wc.hIconSm, app->wc.hIcon);

		if (app->tray.init)
			Shell_NotifyIcon(NIM_DELETE, &app->tray.nid);
	}

	app->tray.init = false;
	memset(&app->tray.nid, 0, sizeof(NOTIFYICONDATA));
}

void MTY_AppNotification(MTY_App *app, const char *title, const char *msg)
{
	if (!title && !msg)
		return;

	HWND hwnd = app_get_main_hwnd(app);
	if (!hwnd)
		return;

	app_tray_set_nid(&app->tray.nid, NIF_INFO | NIF_REALTIME, hwnd, app->wc.hIconSm, app->wc.hIcon);

	if (title)
		MTY_MultiToWide(title, app->tray.nid.szInfoTitle, sizeof(app->tray.nid.szInfoTitle) / sizeof(wchar_t));

	if (msg)
		MTY_MultiToWide(msg, app->tray.nid.szInfo, sizeof(app->tray.nid.szInfo) / sizeof(wchar_t));

	Shell_NotifyIcon(NIM_MODIFY, &app->tray.nid);
}


// Message processing

static LRESULT CALLBACK app_hwnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

static void app_set_button_coords(HWND hwnd, MTY_Event *evt)
{
	POINT p = {0};
	GetCursorPos(&p);
	ScreenToClient(hwnd, &p);

	evt->button.x = p.x;
	evt->button.y = p.y;
}

static void app_ri_relative_mouse(MTY_App *app, HWND hwnd, const RAWINPUT *ri, MTY_Event *evt)
{
	const RAWMOUSE *mouse = &ri->data.mouse;

	if (mouse->lLastX != 0 || mouse->lLastY != 0) {
		if (mouse->usFlags & MOUSE_MOVE_ABSOLUTE) {
			int32_t x = mouse->lLastX;
			int32_t y = mouse->lLastY;

			// It seems that touch input reports lastX and lastY as screen coordinates,
			// not normalized coordinates between 0-65535 as the documentation says
			if (!app->touch_active) {
				bool virt = (mouse->usFlags & MOUSE_VIRTUAL_DESKTOP) ? true : false;
				int32_t w = GetSystemMetrics(virt ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
				int32_t h = GetSystemMetrics(virt ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
				x = (int32_t) (((float) mouse->lLastX / 65535.0f) * w);
				y = (int32_t) (((float) mouse->lLastY / 65535.0f) * h);
			}

			if (app->last_x != -1 && app->last_y != -1) {
				evt->type = MTY_EVENT_MOTION;
				evt->motion.relative = true;
				evt->motion.synth = true;
				evt->motion.x = (int32_t) (x - app->last_x);
				evt->motion.y = (int32_t) (y - app->last_y);
			}

			app->last_x = x;
			app->last_y = y;

		} else {
			evt->type = MTY_EVENT_MOTION;
			evt->motion.relative = true;
			evt->motion.synth = false;
			evt->motion.x = mouse->lLastX;
			evt->motion.y = mouse->lLastY;
		}
	}

	ULONG b = mouse->usButtonFlags;

	if (b & RI_MOUSE_LEFT_BUTTON_DOWN)   app_hwnd_proc(hwnd, WM_LBUTTONDOWN, 0, 0);
	if (b & RI_MOUSE_LEFT_BUTTON_UP)     app_hwnd_proc(hwnd, WM_LBUTTONUP, 0, 0);
	if (b & RI_MOUSE_MIDDLE_BUTTON_DOWN) app_hwnd_proc(hwnd, WM_MBUTTONDOWN, 0, 0);
	if (b & RI_MOUSE_MIDDLE_BUTTON_UP)   app_hwnd_proc(hwnd, WM_MBUTTONUP, 0, 0);
	if (b & RI_MOUSE_RIGHT_BUTTON_DOWN)  app_hwnd_proc(hwnd, WM_RBUTTONDOWN, 0, 0);
	if (b & RI_MOUSE_RIGHT_BUTTON_UP)    app_hwnd_proc(hwnd, WM_RBUTTONUP, 0, 0);
	if (b & RI_MOUSE_BUTTON_4_DOWN)      app_hwnd_proc(hwnd, WM_XBUTTONDOWN, XBUTTON1 << 16, 0);
	if (b & RI_MOUSE_BUTTON_4_UP)        app_hwnd_proc(hwnd, WM_XBUTTONUP, XBUTTON1 << 16, 0);
	if (b & RI_MOUSE_BUTTON_5_DOWN)      app_hwnd_proc(hwnd, WM_XBUTTONDOWN, XBUTTON2 << 16, 0);
	if (b & RI_MOUSE_BUTTON_5_UP)        app_hwnd_proc(hwnd, WM_XBUTTONUP, XBUTTON2 << 16, 0);
	if (b & RI_MOUSE_WHEEL)              app_hwnd_proc(hwnd, WM_MOUSEWHEEL, mouse->usButtonData << 16, 0);
	if (b & RI_MOUSE_HWHEEL)             app_hwnd_proc(hwnd, WM_MOUSEHWHEEL, mouse->usButtonData << 16, 0);
}

static MTY_Mod app_get_keymod(void)
{
	return
		((GetKeyState(VK_LSHIFT)   & 0x8000) >> 15) |
		((GetKeyState(VK_RSHIFT)   & 0x8000) >> 14) |
		((GetKeyState(VK_LCONTROL) & 0x8000) >> 13) |
		((GetKeyState(VK_RCONTROL) & 0x8000) >> 12) |
		((GetKeyState(VK_LMENU)    & 0x8000) >> 11) |
		((GetKeyState(VK_RMENU)    & 0x8000) >> 10) |
		((GetKeyState(VK_LWIN)     & 0x8000) >> 9)  |
		((GetKeyState(VK_RWIN)     & 0x8000) >> 8)  |
		((GetKeyState(VK_CAPITAL)  & 0x0001) << 8)  |
		((GetKeyState(VK_NUMLOCK)  & 0x0001) << 9);
}

static LRESULT CALLBACK app_ll_keyboard_proc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION && WINDOW_KB_HWND) {
		const KBDLLHOOKSTRUCT *p = (const KBDLLHOOKSTRUCT *) lParam;

		bool intercept = ((p->flags & LLKHF_ALTDOWN) && p->vkCode == VK_TAB) || // ALT + TAB
			(p->vkCode == VK_LWIN || p->vkCode == VK_RWIN) || // Windows Key
			(p->vkCode == VK_APPS) || // Application Key
			(p->vkCode == VK_ESCAPE); // ESC

		if (intercept) {
			LPARAM wproc_lparam = 0;
			wproc_lparam |= (p->scanCode & 0xFF) << 16;
			wproc_lparam |= (p->flags & LLKHF_EXTENDED) ? (1 << 24) : 0;
			wproc_lparam |= (p->flags & LLKHF_UP) ? (1 << 31) : 0;

			SendMessage(WINDOW_KB_HWND, (UINT) wParam, 0, wproc_lparam);
			return 1;
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static void app_apply_clip(MTY_App *app, bool focus)
{
	if (focus) {
		if (app->relative && app->detach != MTY_DETACH_FULL) {
			ClipCursor(&app->clip);

		} else if (app->mgrab && app->detach == MTY_DETACH_NONE) {
			struct window *ctx = app_get_focus_window(app);
			if (ctx) {
				RECT r = {0};
				if (GetClientRect(ctx->hwnd, &r)) {
					ClientToScreen(ctx->hwnd, (POINT *) &r.left);
					ClientToScreen(ctx->hwnd, (POINT *) &r.right);
					ClipCursor(&r);
				}
			}
		} else {
			ClipCursor(NULL);
		}
	} else {
		ClipCursor(NULL);
	}
}

static void app_apply_cursor(MTY_App *app, bool focus)
{
	if (focus && (app->hide_cursor || (app->relative && app->detach == MTY_DETACH_NONE))) {
		app->cursor = NULL;

	} else {
		app->cursor = (app->custom_cursor && !app->default_cursor) ? app->custom_cursor :
			LoadCursor(NULL, IDC_ARROW);
	}

	if (GetCursor() != app->cursor)
		SetCursor(app->cursor);
}

static void app_apply_mouse_ri(MTY_App *app, bool focus)
{
	if (app->relative && !app->pen_in_range) {
		if (focus) {
			if (app->detach == MTY_DETACH_FULL) {
				app_register_raw_input(0x01, 0x02, 0, NULL);

			} else {
				app_register_raw_input(0x01, 0x02, RIDEV_NOLEGACY, NULL);
			}
		} else {
			app_register_raw_input(0x01, 0x02, 0, NULL);
		}
	} else {
		// Exiting raw input generates a single WM_MOUSEMOVE, filter it
		app->filter_move = true;
		app_register_raw_input(0x01, 0x02, RIDEV_REMOVE, NULL);
	}
}

static void app_apply_keyboard_state(MTY_App *app, bool focus)
{
	if (focus && app->kbgrab && app->detach == MTY_DETACH_NONE) {
		if (!app->kbhook) {
			struct window *ctx = app_get_focus_window(app);
			if (ctx) {
				WINDOW_KB_HWND = ctx->hwnd; // Unfortunately this needs to be global
				app->kbhook = SetWindowsHookEx(WH_KEYBOARD_LL, app_ll_keyboard_proc, app->instance, 0);
				if (!app->kbhook) {
					WINDOW_KB_HWND = NULL;
					MTY_Log("'SetWindowsHookEx' failed with error 0x%X", GetLastError());
				}
			}
		}

	} else {
		if (app->kbhook) {
			if (!UnhookWindowsHookEx(app->kbhook))
				MTY_Log("'UnhookWindowsHookEx' failed with error 0x%X", GetLastError());

			WINDOW_KB_HWND = NULL;
			app->kbhook = NULL;
		}
	}
}

static bool app_button_is_pressed(MTY_Button button)
{
	return (GetAsyncKeyState(APP_MOUSE_MAP[button]) & 0x8000) ? true : false;
}

static void app_fix_mouse_buttons(MTY_App *ctx)
{
	if (ctx->buttons == 0)
		return;

	bool flipped = GetSystemMetrics(SM_SWAPBUTTON);
	MTY_Button buttons = ctx->buttons;

	for (MTY_Button x = 0; buttons > 0 && x < APP_MOUSE_MAX; x++) {
		if (buttons & 1) {
			MTY_Button b = flipped && x == MTY_BUTTON_LEFT ? MTY_BUTTON_RIGHT :
				flipped && x == MTY_BUTTON_RIGHT ? MTY_BUTTON_LEFT : x;

			if (!app_button_is_pressed(b)) {
				MTY_Event evt = {0};
				evt.type = MTY_EVENT_BUTTON;
				evt.button.button = x;

				ctx->event_func(&evt, ctx->opaque);
				ctx->buttons &= ~(1 << x);
			}
		}

		buttons >>= 1;
	}
}

static LRESULT app_custom_hwnd_proc(struct window *ctx, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	MTY_App *app = ctx->app;

	MTY_Event evt = {0};
	evt.window = ctx->window;

	LRESULT r = 0;
	bool creturn = false;
	bool defreturn = false;
	bool pen_active = app->pen_enabled && app->pen_in_range;
	char drop_name[MTY_PATH_MAX];

	switch (msg) {
		case WM_CLOSE:
			evt.type = MTY_EVENT_CLOSE;
			break;
		case WM_SIZE:
			app->state++;
			evt.type = MTY_EVENT_SIZE;
			break;
		case WM_MOVE:
			evt.type = MTY_EVENT_MOVE;
			break;
		case WM_SETCURSOR:
			if (LOWORD(lparam) == HTCLIENT) {
				app_apply_cursor(app, MTY_AppIsActive(app));
				creturn = true;
				r = TRUE;
			}
			break;
		case WM_DISPLAYCHANGE: {
			// A display change can bork DXGI ResizeBuffers, forcing a dummy 1px move puts it back to normal
			RECT wrect = {0};
			GetWindowRect(hwnd, &wrect);
			SetWindowPos(hwnd, 0, wrect.left + 1, wrect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			SetWindowPos(hwnd, 0, wrect.left, wrect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			break;
		}
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			evt.type = MTY_EVENT_FOCUS;
			evt.focus = msg == WM_SETFOCUS;
			app->state++;
			break;
		case WM_QUERYENDSESSION:
		case WM_ENDSESSION:
			evt.type = MTY_EVENT_SHUTDOWN;
			break;
		case WM_GETMINMAXINFO: {
			float scale = app_hwnd_get_scale(hwnd);
			MINMAXINFO *info = (MINMAXINFO *) lparam;
			info->ptMinTrackSize.x = lrint((float) ctx->min_width * scale);
			info->ptMinTrackSize.y = lrint((float) ctx->min_height * scale);
			creturn = true;
			r = 0;
			break;
		}
		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
			evt.type = MTY_EVENT_KEYBOARD;
			evt.key.mod = app_get_keymod();
			evt.key.pressed = !(lparam >> 31);
			evt.key.key = lparam >> 16 & 0xFF;
			if (lparam >> 24 & 0x01)
				evt.key.key |= 0x0100;

			// Print Screen needs a synthesized WM_KEYDOWN
			if (!evt.key.pressed && evt.key.key == MTY_KEY_PRINT_SCREEN)
				app_custom_hwnd_proc(ctx, hwnd, WM_KEYDOWN, wparam, lparam & 0x7FFFFFFF);
			break;
		case WM_MOUSEMOVE:
			if (!app->filter_move && !pen_active && (!app->relative || app_hwnd_active(hwnd))) {
				evt.type = MTY_EVENT_MOTION;
				evt.motion.relative = false;
				evt.motion.synth = false;
				evt.motion.x = GET_X_LPARAM(lparam);
				evt.motion.y = GET_Y_LPARAM(lparam);
			}

			app->filter_move = false;
			break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			if (!pen_active) {
				evt.type = MTY_EVENT_BUTTON;
				evt.button.button = MTY_BUTTON_LEFT;
				evt.button.pressed = msg == WM_LBUTTONDOWN;
			}
			break;
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			if (!pen_active) {
				evt.type = MTY_EVENT_BUTTON;
				evt.button.button = MTY_BUTTON_RIGHT;
				evt.button.pressed = msg == WM_RBUTTONDOWN;
			}
			break;
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
			evt.type = MTY_EVENT_BUTTON;
			evt.button.button = MTY_BUTTON_MIDDLE;
			evt.button.pressed = msg == WM_MBUTTONDOWN;
			break;
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP: {
			UINT button = GET_XBUTTON_WPARAM(wparam);
			evt.type = MTY_EVENT_BUTTON;
			evt.button.button = button == XBUTTON1 ? MTY_BUTTON_X1 : MTY_BUTTON_X2;
			evt.button.pressed = msg == WM_XBUTTONDOWN;
			break;
		}
		case WM_MOUSEWHEEL:
			evt.type = MTY_EVENT_SCROLL;
			evt.scroll.x = 0;
			evt.scroll.y = GET_WHEEL_DELTA_WPARAM(wparam);
			break;
		case WM_MOUSEHWHEEL:
			evt.type = MTY_EVENT_SCROLL;
			evt.scroll.x = GET_WHEEL_DELTA_WPARAM(wparam);
			evt.scroll.y = 0;
			break;
		case WM_POINTERENTER: {
			if (!_GetPointerType)
				break;

			UINT32 id = GET_POINTERID_WPARAM(wparam);

			POINTER_INPUT_TYPE type = PT_POINTER;
			if (_GetPointerType(id, &type)) {
				app->pen_in_range = type == PT_PEN;
				app->touch_active = type == PT_TOUCH || type == PT_TOUCHPAD;
			}
			break;
		}
		case WM_POINTERLEAVE:
			app->pen_in_range = false;
			app->touch_active = false;
			break;
		case WM_POINTERUPDATE:
			if (!app->pen_enabled || !_GetPointerType || !_GetPointerPenInfo)
				break;

			UINT32 id = GET_POINTERID_WPARAM(wparam);

			POINTER_INPUT_TYPE type = PT_POINTER;
			if (!_GetPointerType(id, &type) || type != PT_PEN)
				break;

			POINTER_PEN_INFO ppi = {0};
			if (!_GetPointerPenInfo(id, &ppi))
				break;

			POINTER_INFO *pi = &ppi.pointerInfo;
			evt.type = MTY_EVENT_PEN;
			evt.pen.pressure = (uint16_t) ppi.pressure;
			evt.pen.rotation = (uint16_t) ppi.rotation;
			evt.pen.tiltX = (int8_t) ppi.tiltX;
			evt.pen.tiltY = (int8_t) ppi.tiltY;

			ScreenToClient(hwnd, &pi->ptPixelLocation);

			if (pi->ptPixelLocation.x < 0)
				pi->ptPixelLocation.x = 0;

			if (pi->ptPixelLocation.y < 0)
				pi->ptPixelLocation.y = 0;

			evt.pen.x = (uint16_t) pi->ptPixelLocation.x;
			evt.pen.y = (uint16_t) pi->ptPixelLocation.y;

			evt.pen.flags = 0;

			if (!(pi->pointerFlags & POINTER_FLAG_INRANGE))
				evt.pen.flags |= MTY_PEN_FLAG_LEAVE;

			if (pi->pointerFlags & POINTER_FLAG_INCONTACT)
				evt.pen.flags |= MTY_PEN_FLAG_TOUCHING;

			if (ppi.penFlags & PEN_FLAG_INVERTED)
				evt.pen.flags |= MTY_PEN_FLAG_INVERTED;

			if (ppi.penFlags & PEN_FLAG_ERASER)
				evt.pen.flags |= MTY_PEN_FLAG_ERASER;

			if (ppi.penFlags & PEN_FLAG_BARREL)
				evt.pen.flags |= MTY_PEN_FLAG_BARREL;

			defreturn = true;
			break;
		case WM_HOTKEY:
			if (!app->ghk_disabled) {
				evt.type = MTY_EVENT_HOTKEY;
				evt.hotkey = (uint32_t) (uintptr_t) MTY_HashGetInt(app->ghotkey, (uint32_t) wparam);
			}
			break;
		case WM_CHAR: {
			wchar_t wstr[2] = {0};
			wstr[0] = (wchar_t) wparam;
			if (MTY_WideToMulti(wstr, evt.text, 8))
				evt.type = MTY_EVENT_TEXT;
			break;
		}
		case WM_CLIPBOARDUPDATE: {
			DWORD cb_seq = GetClipboardSequenceNumber();
			if (cb_seq != app->cb_seq) {
				evt.type = MTY_EVENT_CLIPBOARD;
				app->cb_seq = cb_seq;
			}
			break;
		}
		case WM_DROPFILES:
			wchar_t namew[MTY_PATH_MAX];

			if (DragQueryFile((HDROP) wparam, 0, namew, MTY_PATH_MAX)) {
				SetForegroundWindow(hwnd);

				MTY_WideToMulti(namew, drop_name, MTY_PATH_MAX);
				evt.drop.name = drop_name;
				evt.drop.data = MTY_ReadFile(drop_name, &evt.drop.size);

				if (evt.drop.data)
					evt.type = MTY_EVENT_DROP;
			}
			break;
		case WM_INPUT:
			UINT rsize = WINDOW_RI_MAX;
			UINT e = GetRawInputData((HRAWINPUT) lparam, RID_INPUT, ctx->ri, &rsize, sizeof(RAWINPUTHEADER));
			if (e == 0 || e == 0xFFFFFFFF) {
				MTY_Log("'GetRawInputData' failed with error 0x%X", e);
				break;
			}

			RAWINPUTHEADER *header = &ctx->ri->header;
			if (header->dwType == RIM_TYPEMOUSE) {
				app_ri_relative_mouse(app, hwnd, ctx->ri, &evt);

			} else if (header->dwType == RIM_TYPEHID) {
				mty_hid_win32_report(app->hid, (intptr_t) ctx->ri->header.hDevice,
					ctx->ri->data.hid.bRawData, ctx->ri->data.hid.dwSizeHid);
			}
			break;
		case WM_INPUT_DEVICE_CHANGE:
			mty_hid_win32_device_change(app->hid, wparam, lparam);
			break;
	}

	// Tray
	app_tray_msg(app, msg, wparam, lparam, &evt);

	// Transform keyboard into hotkey
	if (evt.type == MTY_EVENT_KEYBOARD)
		app_kb_to_hotkey(app, &evt);

	// Record pressed buttons
	if (evt.type == MTY_EVENT_BUTTON) {
		app_set_button_coords(hwnd, &evt);

		if (evt.button.pressed) {
			app->buttons |= 1 << evt.button.button;

		} else {
			app->buttons &= ~(1 << evt.button.button);
		}
	}

	// Process the message
	if (evt.type != MTY_EVENT_NONE) {
		app->event_func(&evt, app->opaque);

		if (evt.type == MTY_EVENT_DROP)
			MTY_Free((void *) evt.drop.data);

		if (!defreturn)
			return creturn ? r : 0;
	}

	return creturn ? r : DefWindowProc(hwnd, msg, wparam, lparam);
}

static LRESULT CALLBACK app_hwnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
		case WM_NCCREATE:
			SetWindowLongPtr(hwnd, 0, (LONG_PTR) ((CREATESTRUCT *) lparam)->lpCreateParams);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			struct window *ctx = (struct window *) GetWindowLongPtr(hwnd, 0);
			if (ctx && hwnd)
				return app_custom_hwnd_proc(ctx, hwnd, msg, wparam, lparam);
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}


// Clipboard

char *MTY_AppGetClipboard(MTY_App *app)
{
	struct window *ctx = app_get_main_window(app);
	if (!ctx)
		return NULL;

	char *text = NULL;

	if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		if (OpenClipboard(ctx->hwnd)) {
			HGLOBAL clipbuffer = GetClipboardData(CF_UNICODETEXT);

			if (clipbuffer) {
				wchar_t *locked = GlobalLock(clipbuffer);

				if (locked) {
					text = MTY_WideToMultiD(locked);
					GlobalUnlock(clipbuffer);
				}
			}

			CloseClipboard();
		}
	}

	return text;
}

void MTY_AppSetClipboard(MTY_App *app, const char *text)
{
	struct window *ctx = app_get_main_window(app);
	if (!ctx)
		return;

	if (OpenClipboard(ctx->hwnd)) {
		wchar_t *wtext = MTY_MultiToWideD(text);
		size_t size = (wcslen(wtext) + 1) * sizeof(wchar_t);

		HGLOBAL mem =  GlobalAlloc(GMEM_MOVEABLE, size);
		if (mem) {
			wchar_t *locked = GlobalLock(mem);

			if (locked) {
				memcpy(locked, wtext, size);
				GlobalUnlock(mem);

				EmptyClipboard();
				SetClipboardData(CF_UNICODETEXT, mem);
			}
		}

		CloseClipboard();
		app->cb_seq = GetClipboardSequenceNumber();
	}
}


// Cursor

static void app_set_png_cursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
	HDC dc = NULL;
	ICONINFO ii = {0};
	void *mask = NULL;

	uint32_t width = 0;
	uint32_t height = 0;
	uint8_t *rgba = MTY_DecompressImage(image, size, &width, &height);
	if (!rgba)
		goto except;

	size_t pad = sizeof(size_t) * 8;
	size_t mask_len = width + ((pad - width % pad) / 8) * height;
	mask = MTY_Alloc(mask_len, 1);
	memset(mask, 0xFF, mask_len);

	dc = GetDC(NULL);
	if (!dc) {
		MTY_Log("'GetDC' failed");
		goto except;
	}

	BITMAPV4HEADER bi = {0};
	bi.bV4Size = sizeof(BITMAPV4HEADER);
	bi.bV4Width = width;
	bi.bV4Height = -((int32_t) height);
	bi.bV4Planes = 1;
	bi.bV4BitCount = 32;
	bi.bV4V4Compression = BI_BITFIELDS;
	bi.bV4AlphaMask = 0xFF000000;
	bi.bV4RedMask   = 0x00FF0000;
	bi.bV4GreenMask = 0x0000FF00;
	bi.bV4BlueMask  = 0x000000FF;

	uint8_t *mem = NULL;
	ii.xHotspot = hotX;
	ii.yHotspot = hotY;
	ii.hbmColor = CreateDIBSection(dc, (BITMAPINFO *) &bi, DIB_RGB_COLORS, &mem, NULL, 0);
	if (!ii.hbmColor) {
		MTY_Log("'CreateDIBSection' failed to create color bitmap");
		goto except;
	}

	ii.hbmMask = CreateBitmap(width, height, 1, 1, mask);
	if (!ii.hbmMask) {
		MTY_Log("'CreateBitmap' failed to create mask bitmap");
		goto except;
	}

	uint32_t pitch = width * 4;
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < pitch; x += 4) {
			mem[y * pitch + x + 0] = rgba[y * pitch + x + 2];
			mem[y * pitch + x + 1] = rgba[y * pitch + x + 1];
			mem[y * pitch + x + 2] = rgba[y * pitch + x + 0];
			mem[y * pitch + x + 3] = rgba[y * pitch + x + 3];
		}
	}

	app->custom_cursor = CreateIconIndirect(&ii);
	if (!app->custom_cursor) {
		MTY_Log("'CreateIconIndirect' failed with error 0x%X", GetLastError());
		goto except;
	}

	except:

	if (dc)
		ReleaseDC(NULL, dc);

	if (ii.hbmColor)
		DeleteObject(ii.hbmColor);

	if (ii.hbmMask)
		DeleteObject(ii.hbmMask);

	MTY_Free(mask);
	MTY_Free(rgba);
}

void MTY_AppSetPNGCursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
	if (app->custom_cursor) {
		DestroyIcon(app->custom_cursor);
		app->custom_cursor = NULL;
		app->state++;
	}

	if (image && size > 0) {
		app_set_png_cursor(app, image, size, hotX, hotY);
		app->state++;
	}
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
	if (app->default_cursor != useDefault) {
		app->default_cursor = useDefault;
		app->state++;
	}
}

void MTY_AppShowCursor(MTY_App *ctx, bool show)
{
	if (ctx->hide_cursor == show) {
		ctx->hide_cursor = !show;
		ctx->state++;
	}
}

bool MTY_AppCanWarpCursor(MTY_App *ctx)
{
	return true;
}


// App

static void app_mty_hid_connect(struct hdevice *device, void *opaque)
{
	MTY_App *ctx = opaque;

	mty_hid_driver_init(device);

	MTY_Event evt = {0};
	evt.type = MTY_EVENT_CONNECT;
	evt.controller.vid = mty_hid_device_get_vid(device);
	evt.controller.pid = mty_hid_device_get_pid(device);
	evt.controller.id = mty_hid_device_get_id(device);

	ctx->event_func(&evt, ctx->opaque);
}

static void app_mty_hid_disconnect(struct hdevice *device, void *opaque)
{
	MTY_App *ctx = opaque;

	MTY_Event evt = {0};
	evt.type = MTY_EVENT_DISCONNECT;
	evt.controller.vid = mty_hid_device_get_vid(device);
	evt.controller.pid = mty_hid_device_get_pid(device);
	evt.controller.id = mty_hid_device_get_id(device);

	ctx->event_func(&evt, ctx->opaque);
}

static void app_mty_hid_report(struct hdevice *device, const void *buf, size_t size, void *opaque)
{
	MTY_App *ctx = opaque;

	MTY_Event evt = {0};
	mty_hid_driver_state(device, buf, size, &evt);

	// Prevent gamepad input while in the background
	if (evt.type == MTY_EVENT_CONTROLLER && MTY_AppIsActive(ctx))
		ctx->event_func(&evt, ctx->opaque);
}

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_EventFunc eventFunc, void *opaque)
{
	bool r = true;

	MTY_App *app = MTY_Alloc(1, sizeof(MTY_App));
	app->app_func = appFunc;
	app->event_func = eventFunc;
	app->opaque = opaque;
	app->hotkey = MTY_HashCreate(0);
	app->ghotkey = MTY_HashCreate(0);
	app->instance = GetModuleHandle(NULL);
	if (!app->instance) {
		r = false;
		MTY_Log("'GetModuleHandle' failed with error 0x%X", GetLastError());
		goto except;
	}

	app->wc.cbSize = sizeof(WNDCLASSEX);
	app->wc.cbWndExtra = sizeof(struct window *);
	app->wc.lpfnWndProc = app_hwnd_proc;
	app->wc.hInstance = app->instance;
	app->wc.lpszClassName = WINDOW_CLASS_NAME;

	app->cursor = LoadCursor(NULL, IDC_ARROW);

	wchar_t path[MTY_PATH_MAX];
	GetModuleFileName(app->instance, path, MTY_PATH_MAX);
	ExtractIconEx(path, 0, &app->wc.hIcon, &app->wc.hIconSm, 1);

	app->class = RegisterClassEx(&app->wc);
	if (app->class == 0) {
		r = false;
		MTY_Log("'RegisterClassEx' failed with error 0x%X", GetLastError());
		goto except;
	}

	app->tb_msg = RegisterWindowMessage(L"TaskbarCreated");

	HMODULE user32 = GetModuleHandle(L"user32.dll");
	HMODULE shcore = GetModuleHandle(L"shcore.dll");
	_GetDpiForMonitor = (void *) GetProcAddress(shcore, "GetDpiForMonitor");
	_GetPointerPenInfo = (void *) GetProcAddress(user32, "GetPointerPenInfo");
	_GetPointerType = (void *) GetProcAddress(user32, "GetPointerType");

	ImmDisableIME(0);

	app->hid = mty_hid_create(app_mty_hid_connect, app_mty_hid_disconnect, app_mty_hid_report, app);

	except:

	if (!r)
		MTY_AppDestroy(&app);

	return app;
}

void MTY_AppDestroy(MTY_App **app)
{
	if (!app || !*app)
		return;

	MTY_App *ctx = *app;

	if (ctx->custom_cursor)
		DestroyIcon(ctx->custom_cursor);

	MTY_AppRemoveTray(ctx);
	MTY_AppEnableScreenSaver(ctx, true);

	app_unregister_global_hotkeys(ctx);

	MTY_HashDestroy(&ctx->hotkey, NULL);
	MTY_HashDestroy(&ctx->ghotkey, NULL);

	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++)
		MTY_WindowDestroy(ctx, x);

	if (ctx->instance && ctx->class != 0)
		UnregisterClass(WINDOW_CLASS_NAME, ctx->instance);

	mty_hid_destroy(&ctx->hid);

	WINDOW_KB_HWND = NULL;

	MTY_Free(ctx);
	*app = NULL;
}

void MTY_AppRun(MTY_App *app)
{
	for (bool cont = true; cont;) {
		struct window *window = app_get_main_window(app);
		if (!window)
			break;

		bool focus = MTY_AppIsActive(app);

		// Keyboard, mouse state changes
		if (app->prev_state != app->state) {
			app_apply_clip(app, focus);
			app_apply_cursor(app, focus);
			app_apply_mouse_ri(app, focus);
			app_apply_keyboard_state(app, focus);

			app->prev_state = app->state;
		}

		// XInput
		if (focus)
			mty_hid_xinput_state(app->hid, app->event_func, app->opaque);

		// Tray retry in case of failure
		app_tray_retry(app, window);

		// Poll messages belonging to the current (main) thread
		for (MSG msg; PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Mouse button state reconciliation
		app_fix_mouse_buttons(app);

		cont = app->app_func(app->opaque);

		if (app->timeout > 0)
			MTY_Sleep(app->timeout);
	}
}

void MTY_AppSetTimeout(MTY_App *app, uint32_t timeout)
{
	app->timeout = timeout;
}

void MTY_AppDetach(MTY_App *app, MTY_Detach type)
{
	if (app->detach != type) {
		app->detach = type;
		app->state++;
	}
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
	return app->detach;
}

void MTY_AppEnableScreenSaver(MTY_App *app, bool enable)
{
	if (!SetThreadExecutionState(ES_CONTINUOUS | (!enable ? ES_DISPLAY_REQUIRED : 0)))
		MTY_Log("'SetThreadExecutionState' failed");
}

bool MTY_AppKeyboardIsGrabbed(MTY_App *ctx)
{
	return ctx->kbgrab;
}

void MTY_AppGrabKeyboard(MTY_App *app, bool grab)
{
	if (app->kbgrab != grab) {
		app->kbgrab = grab;
		app->state++;
	}
}

bool MTY_AppMouseIsGrabbed(MTY_App *ctx)
{
	return ctx->mgrab;
}

void MTY_AppGrabMouse(MTY_App *app, bool grab)
{
	if (app->mgrab != grab) {
		app->mgrab = grab;
		app->state++;
	}
}

void MTY_AppSetRelativeMouse(MTY_App *app, bool relative)
{
	if (relative && !app->relative) {
		app->relative = true;
		app->last_x = app->last_y = -1;

		POINT p = {0};
		GetCursorPos(&p);
		app->clip.left = p.x;
		app->clip.right = p.x + 1;
		app->clip.top = p.y;
		app->clip.bottom = p.y + 1;

	} else if (!relative && app->relative) {
		app->relative = false;
		app->last_x = app->last_y = -1;
	}

	bool focus = MTY_AppIsActive(app);
	app_apply_mouse_ri(app, focus);
	app_apply_cursor(app, focus);
	app_apply_clip(app, focus);
}

bool MTY_AppGetRelativeMouse(MTY_App *app)
{
	return app->relative;
}

bool MTY_AppIsActive(MTY_App *app)
{
	bool r = false;

	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++)
		r = r || MTY_WindowIsActive(app, x);

	return r;
}

void MTY_AppActivate(MTY_App *app, bool active)
{
	HWND hwnd = app_get_main_hwnd(app);
	if (!hwnd)
		return;

	app_hwnd_activate(hwnd, active);
}

void MTY_AppControllerRumble(MTY_App *app, uint32_t id, uint16_t low, uint16_t high)
{
	if (id < 4) {
		mty_hid_xinput_rumble(app->hid, id, low, high);

	} else {
		mty_hid_driver_rumble(app->hid, id, low, high);
	}
}

bool MTY_AppPenIsEnabled(MTY_App *ctx)
{
	return ctx->pen_enabled;
}

void MTY_AppEnablePen(MTY_App *ctx, bool enable)
{
	ctx->pen_enabled = enable;
}


// Window

static void window_client_to_full(int32_t *width, int32_t *height)
{
	RECT rect = {0};
	rect.right = *width;
	rect.bottom = *height;
	if (AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0)) {
		*width = rect.right - rect.left;
		*height = rect.bottom - rect.top;
	}
}

MTY_Window MTY_WindowCreate(MTY_App *app, const char *title, const MTY_WindowDesc *desc)
{
	MTY_Window window = -1;
	wchar_t *titlew = NULL;
	bool r = true;

	window = app_find_open_window(app);
	if (window == -1) {
		r = false;
		MTY_Log("Maximum windows (MTY_WINDOW_MAX) of %u reached", MTY_WINDOW_MAX);
		goto except;
	}

	struct window *ctx = MTY_Alloc(1, sizeof(struct window));
	app->windows[window] = ctx;

	ctx->ri = MTY_Alloc(WINDOW_RI_MAX, 1);
	ctx->app = app;
	ctx->window = window;
	ctx->min_width = desc->minWidth;
	ctx->min_height = desc->minHeight;

	RECT rect = {0};
	DWORD style = WS_OVERLAPPEDWINDOW;
	HWND desktop = GetDesktopWindow();

	GetWindowRect(desktop, &rect);
	int32_t desktop_height = rect.bottom - rect.top;
	int32_t desktop_width = rect.right - rect.left;

	float scale = app_hwnd_get_scale(desktop);
	int32_t width = desktop_width;
	int32_t height = desktop_height;
	int32_t x = lrint((float) desc->x * scale);
	int32_t y = lrint((float) desc->y * scale);

	if (desc->fullscreen) {
		style = WS_POPUP;
		x = rect.left;
		y = rect.top;

	} else {
		mty_wsize_client(desc, scale, desktop_height, &x, &y, &width, &height);
		window_client_to_full(&width, &height);

		if (desc->position == MTY_POSITION_CENTER)
			mty_wsize_center(rect.left, rect.top, desktop_width, desktop_height, &x, &y, &width, &height);
	}

	titlew = MTY_MultiToWideD(title);

	ctx->hwnd = CreateWindowEx(0, WINDOW_CLASS_NAME, titlew, style,
		x, y, width, height, NULL, NULL, app->instance, ctx);
	if (!ctx->hwnd) {
		r = false;
		MTY_Log("'CreateWindowEx' failed with error 0x%X", GetLastError());
		goto except;
	}

	if (!desc->hidden)
		app_hwnd_activate(ctx->hwnd, true);

	if (desc->api != MTY_GFX_NONE) {
		if (!MTY_WindowSetGFX(app, window, desc->api, desc->vsync)) {
			r = false;
			goto except;
		}
	}

	DragAcceptFiles(ctx->hwnd, TRUE);

	if (window == 0) {
		AddClipboardFormatListener(ctx->hwnd);
		mty_hid_win32_listen(ctx->hwnd);
	}

	except:

	MTY_Free(titlew);

	if (!r) {
		MTY_WindowDestroy(app, window);
		window = -1;
	}

	return window;
}

void MTY_WindowDestroy(MTY_App *app, MTY_Window window)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	struct window *focus = app_get_focus_window(app);
	if (ctx == focus) {
		MTY_AppGrabKeyboard(app, false);
		MTY_AppGrabMouse(app, false);
	}

	if (ctx->hwnd)
		DestroyWindow(ctx->hwnd);

	if (ctx->gfx_ctx)
		MTY_Log("Window destroyed with GFX still attached");

	MTY_Free(ctx->ri);
	MTY_Free(ctx);
	app->windows[window] = NULL;
}

void MTY_WindowSetTitle(MTY_App *app, MTY_Window window, const char *title)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	wchar_t *titlew = MTY_MultiToWideD(title);
	SetWindowText(ctx->hwnd, titlew);
	MTY_Free(titlew);
}

bool MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	RECT rect = {0};
	if (GetClientRect(ctx->hwnd, &rect)) {
		*width = rect.right - rect.left;
		*height = rect.bottom - rect.top;
		return true;
	}

	return false;
}

static bool window_get_monitor_info(HWND hwnd, MONITORINFOEX *info)
{
	HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

	if (mon) {
		memset(info, 0, sizeof(MONITORINFOEX));
		info->cbSize = sizeof(MONITORINFOEX);

		return GetMonitorInfo(mon, (LPMONITORINFO) info);
	}

	return false;
}

bool MTY_WindowGetScreenSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	MONITORINFOEX info = {0};
	if (window_get_monitor_info(ctx->hwnd, &info)) {
		*width = info.rcMonitor.right - info.rcMonitor.left;
		*height = info.rcMonitor.bottom - info.rcMonitor.top;
		return true;
	}

	return false;
}

float MTY_WindowGetScale(MTY_App *app, MTY_Window window)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return 1.0f;

	return app_hwnd_get_scale(ctx->hwnd);
}

void MTY_WindowSetFullscreen(MTY_App *app, MTY_Window window, bool fullscreen)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	if (fullscreen && !MTY_WindowIsFullscreen(app, window)) {
		MONITORINFOEX info = {0};
		if (window_get_monitor_info(ctx->hwnd, &info)) {
			RECT r = {0};
			if (GetWindowRect(ctx->hwnd, &r)) {
				ctx->x = r.left;
				ctx->y = r.top;
			}

			if (GetClientRect(ctx->hwnd, &r)) {
				ctx->width = r.right - r.left;
				ctx->height = r.bottom - r.top;
			}

			window_client_to_full(&ctx->width, &ctx->height);

			uint32_t x = info.rcMonitor.left;
			uint32_t y = info.rcMonitor.top;
			uint32_t w = info.rcMonitor.right - info.rcMonitor.left;
			uint32_t h = info.rcMonitor.bottom - info.rcMonitor.top;

			SetWindowLongPtr(ctx->hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
			SetWindowPos(ctx->hwnd, HWND_TOP, x, y, w, h, SWP_FRAMECHANGED);
		}

	} else if (!fullscreen && MTY_WindowIsFullscreen(app, window)) {
		SetWindowLongPtr(ctx->hwnd, GWL_STYLE, WS_VISIBLE | WS_OVERLAPPEDWINDOW);
		SetWindowPos(ctx->hwnd, HWND_TOP, ctx->x, ctx->y, ctx->width, ctx->height, SWP_FRAMECHANGED);

		PostMessage(ctx->hwnd, WM_SETICON, ICON_BIG, GetClassLongPtr(ctx->hwnd, GCLP_HICON));
		PostMessage(ctx->hwnd, WM_SETICON, ICON_SMALL, GetClassLongPtr(ctx->hwnd, GCLP_HICONSM));
	}
}

bool MTY_WindowIsFullscreen(MTY_App *app, MTY_Window window)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	return GetWindowLongPtr(ctx->hwnd, GWL_STYLE) & WS_POPUP;
}

void MTY_WindowActivate(MTY_App *app, MTY_Window window, bool active)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	app_hwnd_activate(ctx->hwnd, active);
}

void MTY_WindowWarpCursor(MTY_App *app, MTY_Window window, uint32_t x, uint32_t y)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	POINT p;
	p.x = x;
	p.y = y;

	ClipCursor(NULL);
	if (ClientToScreen(ctx->hwnd, &p))
		SetCursorPos(p.x, p.y);

	MTY_AppSetRelativeMouse(app, false);
}

bool MTY_WindowIsVisible(MTY_App *app, MTY_Window window)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	return app_hwnd_visible(ctx->hwnd);
}

bool MTY_WindowIsActive(MTY_App *app, MTY_Window window)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	return app_hwnd_active(ctx->hwnd);
}

bool MTY_WindowExists(MTY_App *app, MTY_Window window)
{
	return app_get_window(app, window) ? true : false;
}


// Window Private

void mty_window_set_gfx(MTY_App *app, MTY_Window window, MTY_GFX api, struct gfx_ctx *gfx_ctx)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	ctx->api = api;
	ctx->gfx_ctx = gfx_ctx;
}

MTY_GFX mty_window_get_gfx(MTY_App *app, MTY_Window window, struct gfx_ctx **gfx_ctx)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return MTY_GFX_NONE;

	if (gfx_ctx)
		*gfx_ctx = ctx->gfx_ctx;

	return ctx->api;
}

void *mty_window_get_native(MTY_App *app, MTY_Window window)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return NULL;

	return (void *) ctx->hwnd;
}


// Misc

void *MTY_GLGetProcAddress(const char *name)
{
	void *p = wglGetProcAddress(name);

	if (!p)
		p = GetProcAddress(GetModuleHandleA("opengl32.dll"), name);

	return p;
}

void MTY_MessageBox(const char *title, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char *msg = MTY_VsprintfD(fmt, args);

	va_end(args);

	WCHAR *wmsg = MTY_MultiToWideD(msg);
	WCHAR *wtitle = MTY_MultiToWideD(title);

	MessageBox(NULL, wmsg, wtitle, MB_OK);

	MTY_Free(wmsg);
	MTY_Free(wtitle);
	MTY_Free(msg);
}

void MTY_SetAppID(const char *id)
{
	WCHAR *wid = MTY_MultiToWideD(id);

	SetCurrentProcessExplicitAppUserModelID(wid);

	MTY_Free(wid);
}


// Unimplemented

void MTY_AppShowSoftKeyboard(MTY_App *app, bool show)
{
}

bool MTY_AppSoftKeyboardIsShowing(MTY_App *app)
{
	return false;
}

void MTY_AppSetOrientation(MTY_App *app, MTY_Orientation orientation)
{
}

MTY_GFXState MTY_WindowGFXState(MTY_App *app, MTY_Window window)
{
	return MTY_GFX_STATE_NORMAL;
}

void MTY_AppSetInputMode(MTY_App *ctx, MTY_Input mode)
{
}
