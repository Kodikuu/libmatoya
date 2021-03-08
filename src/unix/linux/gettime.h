// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <time.h>
#include <errno.h>

static void mty_get_time(struct timespec *ts)
{
	if (clock_gettime(CLOCK_REALTIME, ts) != 0)
		MTY_Fatal("'clock_gettime' failed with errno %d", errno);
}
