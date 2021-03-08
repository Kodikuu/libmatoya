// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "matoya.h"
#include "gfx/mod-ctx.h"

void mty_window_set_gfx(MTY_App *app, MTY_Window window, MTY_GFX api, struct gfx_ctx *gfx_ctx);
MTY_GFX mty_window_get_gfx(MTY_App *app, MTY_Window window, struct gfx_ctx **gfx_ctx);
void *mty_window_get_native(MTY_App *app, MTY_Window window);
