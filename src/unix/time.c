// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include "tlocal.h"
#include "sleep.h"
#include "timestamp.h"

static MTY_TLOCAL bool TIME_FREQ_INIT;
static MTY_TLOCAL float TIME_FREQUENCY;

int64_t MTY_GetTime(void)
{
	return mty_timestamp();
}

float MTY_TimeDiff(int64_t begin, int64_t end)
{
	if (!TIME_FREQ_INIT) {
		TIME_FREQUENCY = mty_frequency();
		TIME_FREQ_INIT = true;
	}

	return (float) (end - begin) * TIME_FREQUENCY;
}

void MTY_Sleep(uint32_t timeout)
{
	mty_sleep(timeout);
}

void MTY_SetTimerResolution(uint32_t res)
{
}

void MTY_RevertTimerResolution(uint32_t res)
{
}
