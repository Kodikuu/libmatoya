// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <string.h>

#include <unistd.h>

static bool mty_proc_name(char *name, size_t size)
{
	ssize_t n = readlink("/proc/self/exe", name, size - 1);
	if (n < 0) {
		MTY_Log("'readlink' failed with errno %d", errno);
		return false;
	}

	return true;
}
