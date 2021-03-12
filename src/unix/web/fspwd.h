// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#pragma once

struct passwd {
	const char *pw_dir;
};

static struct passwd __HOME = {"/"};

#define getuid() 0
#define getpwuid(uid) (&__HOME)
