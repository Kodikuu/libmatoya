// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <mach/mach_time.h>

static int64_t mty_timestamp(void)
{
	return mach_absolute_time();
}

static float mty_frequency(void)
{
	mach_timebase_info_data_t timebase;
	kern_return_t e = mach_timebase_info(&timebase);
	if (e != KERN_SUCCESS)
		MTY_Fatal("'mach_timebase_info' failed with error %d", e);

	return timebase.numer / timebase.denom / 1000000.0f;
}
