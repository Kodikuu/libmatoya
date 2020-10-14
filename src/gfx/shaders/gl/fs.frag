// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifdef GL_ES
	precision mediump float;
#endif

varying vec2 vs_texcoord;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;

// scale, width, height, vp_height
uniform vec4 fcb;

// filter, effect, format
uniform ivec3 icb;

void yuv_to_rgba(float y, float u, float v, out vec4 rgba)
{
	// Using "RGB to YCbCr color conversion for HDTV" (ITU-R BT.709)

	y = (y - 0.0625) * 1.164;
	u = u - 0.5;
	v = v - 0.5;

	float r = y + 1.793 * v;
	float g = y - 0.213 * u - 0.533 * v;
	float b = y + 2.112 * u;

	rgba = vec4(r, g, b, 1.0);
}

void gaussian(int type, float w, float h, inout vec2 uv)
{
	vec2 res = vec2(w, h);
	vec2 p = uv * res;
	vec2 c = floor(p) + 0.5;
	vec2 dist = p - c;

	// Sharp
	if (type == 3) {
		dist = 16.0 * dist * dist * dist * dist * dist;

	// Soft
	} else {
		dist = 4.0 * dist * dist * dist;
	}

	p = c + dist;

	uv = p / res;
}

void scanline(int effect, float y, float h, inout vec4 rgba)
{
	float n = (effect == 1) ? 1.0 : 2.0;

	if (mod(floor(y * h), n * 2.0) < n)
		rgba *= 0.7;
}

void main(void)
{
	float scale = fcb[0];
	float width = fcb[1];
	float height = fcb[2];
	float vp_height = fcb[3];

	int filter = icb[0];
	int effect = icb[1];
	int format = icb[2];

	vec2 uv = vec2(
		vs_texcoord[0] * scale,
		vs_texcoord[1]
	);

	// Gaussian
	if (filter == 3 || filter == 4)
		gaussian(filter, width, height, uv);

	// NV12, NV16
	if (format == 2 || format == 5) {
		float y = texture2D(tex0, uv).r;
		float u = texture2D(tex1, uv).r;
		float v = texture2D(tex1, uv).g;

		yuv_to_rgba(y, u, v, gl_FragColor);

	// I420, I444
	} else if (format == 3 || format == 4) {
		float y = texture2D(tex0, uv).r;
		float u = texture2D(tex1, uv).r;
		float v = texture2D(tex2, uv).r;

		yuv_to_rgba(y, u, v, gl_FragColor);

	// RGBA, BGRA
	} else {
		gl_FragColor = texture2D(tex0, uv);
	}

	// Scanlines
	if (effect == 1 || effect == 2)
		scanline(effect, vs_texcoord[1], vp_height, gl_FragColor);
}
