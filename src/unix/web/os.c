// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>

#include "web.h"

uint32_t MTY_GetPlatform(void)
{
	return MTY_OS_WEB;
}

uint32_t MTY_GetPlatformNoWeb(void)
{
	char platform[64] = "";
	web_platform(platform, 64);

	if (strstr(platform, "Win32"))
		return MTY_OS_WINDOWS;

	if (strstr(platform, "Mac"))
		return MTY_OS_MACOS;

	if (strstr(platform, "Android"))
		return MTY_OS_ANDROID;

	if (strstr(platform, "Linux"))
		return MTY_OS_LINUX;

	if (strstr(platform, "iPhone"))
		return MTY_OS_IOS;

	return MTY_OS_UNKNOWN;
}