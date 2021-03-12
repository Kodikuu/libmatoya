// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include "matoya.h"

#include "gfx/mod-ctx.h"

void MTY_HotkeyToString(MTY_Mod mod, MTY_Key key, char *str, size_t len)
{
}

void MTY_AppSetHotkey(MTY_App *app, MTY_Hotkey mode, MTY_Mod mod, MTY_Key key, uint32_t id)
{
}

void MTY_AppRemoveHotkeys(MTY_App *app, MTY_Hotkey mode)
{
}

char *MTY_AppGetClipboard(MTY_App *app)
{
	return NULL;
}

void MTY_AppSetClipboard(MTY_App *app, const char *text)
{
}

void MTY_AppSetPNGCursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
}

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_MsgFunc msgFunc, void *opaque)
{
	return MTY_Alloc(1, 1);
}

void MTY_AppDestroy(MTY_App **app)
{
	if (!app || !*app)
		return;

	MTY_Free(*app);
	*app = NULL;
}

void MTY_AppRun(MTY_App *app)
{
}

void MTY_AppDetach(MTY_App *app, MTY_Detach type)
{
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
	return MTY_DETACH_NONE;
}

void MTY_AppEnableScreenSaver(MTY_App *app, bool enable)
{
}

void MTY_AppGrabMouse(MTY_App *app, bool grab)
{
}

void MTY_AppSetRelativeMouse(MTY_App *app, bool relative)
{
}

bool MTY_AppGetRelativeMouse(MTY_App *app)
{
	return false;
}

bool MTY_AppIsActive(MTY_App *app)
{
	return true;
}

void MTY_AppActivate(MTY_App *app, bool active)
{
}

void MTY_AppControllerRumble(MTY_App *app, uint32_t id, uint16_t low, uint16_t high)
{
}

MTY_Window MTY_WindowCreate(MTY_App *app, const char *title, const MTY_WindowDesc *desc)
{
	return 0;
}

void MTY_WindowDestroy(MTY_App *app, MTY_Window window)
{
}

void MTY_WindowSetTitle(MTY_App *app, MTY_Window window, const char *title)
{
}

bool MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	return MTY_WindowGetScreenSize(app, window, width, height);
}

bool MTY_WindowGetScreenSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	*width = *height = 500;

	return true;
}

float MTY_WindowGetScale(MTY_App *app, MTY_Window window)
{
	return 1.0f;
}

void MTY_WindowEnableFullscreen(MTY_App *app, MTY_Window window, bool fullscreen)
{
}

bool MTY_WindowIsFullscreen(MTY_App *app, MTY_Window window)
{
	return true;
}

void MTY_WindowActivate(MTY_App *app, MTY_Window window, bool active)
{
}

void MTY_WindowWarpCursor(MTY_App *app, MTY_Window window, uint32_t x, uint32_t y)
{
}

bool MTY_WindowIsVisible(MTY_App *app, MTY_Window window)
{
	return true;
}

bool MTY_WindowIsActive(MTY_App *app, MTY_Window window)
{
	return true;
}

bool MTY_WindowExists(MTY_App *app, MTY_Window window)
{
	return window == 0;
}

void window_set_gfx(MTY_App *app, MTY_Window window, MTY_GFX api, struct gfx_ctx *gfx_ctx)
{
}

MTY_GFX window_get_gfx(MTY_App *app, MTY_Window window, struct gfx_ctx **gfx_ctx)
{
	return MTY_GFX_NONE;
}

void *window_get_native(MTY_App *app, MTY_Window window)
{
	return NULL;
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

void MTY_AppSetOnscreenKeyboard(MTY_App *app, bool enable)
{
}

void MTY_AppSetOrientation(MTY_App *app, MTY_Orientation orientation)
{
}

void MTY_AppEnableGlobalHotkeys(MTY_App *app, bool enable)
{
}

void MTY_AppGrabKeyboard(MTY_App *app, bool grab)
{
}
