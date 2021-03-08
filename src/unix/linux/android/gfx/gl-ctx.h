// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "matoya.h"

void mty_gfx_global_init(void);
void mty_gfx_global_destroy(void);
bool mty_gfx_is_ready(void);
void mty_gfx_size(uint32_t *width, uint32_t *height);
void mty_gfx_set_kb_height(int32_t height);
MTY_GFXState mty_gfx_state(void);
