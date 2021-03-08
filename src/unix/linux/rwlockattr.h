// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdlib.h>

#include <pthread.h>

static void mty_rwlockattr_set(pthread_rwlockattr_t *attr)
{
	int32_t e = pthread_rwlockattr_setkind_np(attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
	if (e != 0)
		MTY_Fatal("'pthread_rwlockattr_setkind_np' failed with error %d", e);
}
