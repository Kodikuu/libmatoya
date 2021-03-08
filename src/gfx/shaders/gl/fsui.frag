// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifdef GL_ES
	precision mediump float;
#endif

uniform sampler2D tex;

varying vec2 frag_uv;
varying vec4 frag_col;

void main(void)
{
	gl_FragColor = frag_col * texture2D(tex, frag_uv.st);
}
