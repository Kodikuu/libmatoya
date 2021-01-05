// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "matoya.h"

void web_get_size(uint32_t *width, uint32_t *height);
void web_set_title(const char *title);
void web_create_canvas(void);
void web_raf(MTY_AppFunc func, const void *opaque);
void web_register_drag(void);
float web_get_pixel_ratio(void);
void web_attach_events(MTY_App *app, void *malloc, void *free, void *mouse_motion,
	void *mouse_button, void *mouse_wheel, void *keyboard, void *drop);