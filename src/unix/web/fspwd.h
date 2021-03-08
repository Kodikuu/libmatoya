// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

struct passwd {
	const char *pw_dir;
};

static struct passwd __HOME = {"/"};

#define getuid() 0
#define getpwuid(uid) (&__HOME)
