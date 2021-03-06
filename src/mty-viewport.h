// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <math.h>

static void mty_viewport(uint32_t w, uint32_t h, uint32_t view_w, uint32_t view_h,
	float ar, float scale, float *vp_x, float *vp_y, float *vp_w, float *vp_h)
{
	uint32_t scaled_w = lrint(scale * (float) w);
	uint32_t scaled_h = lrint(scale * (float) h);

	if (scaled_w == 0 || scaled_h == 0 || view_w < scaled_w || view_h < scaled_h)
		scaled_w = view_w;

	*vp_w = (float) scaled_w;
	*vp_h = roundf(*vp_w / ar);

	if (*vp_w > (float) view_w) {
		*vp_w = (float) view_w;
		*vp_h = roundf(*vp_w / ar);
	}

	if (*vp_h > (float) view_h) {
		*vp_h = (float) view_h;
		*vp_w = roundf(*vp_h * ar);
	}

	*vp_x = roundf(((float) view_w - *vp_w) / 2.0f);
	*vp_y = roundf(((float) view_h - *vp_h) / 2.0f);
}
