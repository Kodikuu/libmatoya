// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifdef GL_ES
	precision mediump float;
#endif

attribute vec4 position;
attribute vec2 texcoord;

varying vec2 vs_texcoord;

void main(void)
{
	vs_texcoord = texcoord;
	gl_Position = position;
}
