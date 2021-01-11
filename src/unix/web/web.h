// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "matoya.h"

bool web_has_focus(void);
bool web_is_visible(void);
bool web_get_key(const char *code, char *key, size_t len);
void web_rumble_gamepad(uint32_t id, float low, float high);
void web_show_cursor(bool show);
void web_use_default_cursor(bool use_default);
void web_set_png_cursor(const void *image, size_t size, uint32_t hotX, uint32_t hotY);
void web_set_pointer_lock(bool enable);
bool web_get_pointer_lock(void);
char *web_get_clipboard_text(void);
void web_set_clipboard_text(const char *text);
void web_set_mem_funcs(void *alloc, void *free);
void web_get_size(uint32_t *width, uint32_t *height);
void web_get_screen_size(uint32_t *width, uint32_t *height);
void web_set_title(const char *title);
void web_raf(MTY_App *app, MTY_AppFunc func, void *controller, const void *opaque);
void web_register_drag(void);
float web_get_pixel_ratio(void);
void web_attach_events(MTY_App *app, void *mouse_motion, void *mouse_button,
	void *mouse_wheel, void *keyboard, void *focus, void *drop);
