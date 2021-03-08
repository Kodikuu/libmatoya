// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <sys/time.h>
#include <errno.h>

static void mty_get_time(struct timespec *ts)
{
	struct timeval tv = {0};

	if (gettimeofday(&tv, NULL) != 0)
		MTY_Fatal("'gettimeofday' failed with errno %d", errno);

	ts->tv_nsec = tv.tv_usec * 1000;
	ts->tv_sec = tv.tv_sec + ts->tv_nsec / 1000000000;
	ts->tv_nsec %= 1000000000;
}
