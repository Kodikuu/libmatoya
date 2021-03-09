// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"
#include "tlocal.h"

#include <string.h>
#include <stdio.h>

#define MTY_TLOCAL_MAX (8 * 1024)

static MTY_TLOCAL uint8_t TLOCAL[MTY_TLOCAL_MAX];
static MTY_TLOCAL size_t TLOCAL_OFFSET;

void *mty_tlocal(size_t size)
{
	if (TLOCAL_OFFSET + size > MTY_TLOCAL_MAX)
		TLOCAL_OFFSET = 0;

	void *ptr = TLOCAL + TLOCAL_OFFSET;
	TLOCAL_OFFSET += size;

	return ptr;
}

char *mty_tlocal_strcpy(const char *str)
{
	size_t len = strlen(str) + 1;

	if (len > MTY_TLOCAL_MAX)
		len = MTY_TLOCAL_MAX;

	char *local = mty_tlocal(len);
	snprintf(local, len, "%s", str);

	return local;
}

char *mty_tlocal_strcpyw(wchar_t *wstr)
{
	char *str = MTY_WideToMultiD(wstr);
	char *local = mty_tlocal_strcpy(str);

	MTY_Free(str);

	return local;
}
