// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>

#include "dlopen.h"
#include "tlocal.h"

MTY_SO *MTY_SOLoad(const char *name)
{
	MTY_SO *so = dlopen(name, MTY_DLOPEN_FLAGS);
	if (!so) {
		const char *estr = dlerror();
		if (estr)
			MTY_Log("%s", estr);

		MTY_Log("'dlopen' failed to find '%s'", name);
	}

	return so;
}

void *MTY_SOGetSymbol(MTY_SO *so, const char *name)
{
	void *sym = dlsym(so, name);
	if (!sym) {
		const char *estr = dlerror();
		if (estr)
			MTY_Log("%s", estr);

		MTY_Log("'dlsym' failed to find '%s'", name);
	}

	return sym;
}

void MTY_SOUnload(MTY_SO **so)
{
	if (!so || !*so)
		return;

	int32_t e = dlclose(*so);
	if (e != 0) {
		const char *estr = dlerror();
		if (estr) {
			MTY_Log("'dlclose' failed with error %d: '%s'", e, estr);

		} else {
			MTY_Log("'dlclose' failed with error %d", e);
		}
	}

	*so = NULL;
}

const char *MTY_Hostname(void)
{
	char tmp[MTY_PATH_MAX] = {0};

	int32_t e = gethostname(tmp, MTY_PATH_MAX - 1);
	if (e != 0) {
		MTY_Log("'gethostname' failed with errno %d", errno);
		return "noname";
	}

	return mty_tlocal_strcpy(tmp);
}
