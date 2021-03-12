// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include "matoya.h"

#include <AppKit/AppKit.h>

void MTY_ProtocolHandler(const char *uri, void *token)
{
	NSString *nsuri = [NSString stringWithUTF8String:uri];

	if (strstr(uri, "http") == uri) {
		[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:nsuri]];

	} else {
		[[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:nsuri]];
	}
}

uint32_t MTY_GetPlatform(void)
{
	uint32_t v = MTY_OS_MACOS;

	NSProcessInfo *pInfo = [NSProcessInfo processInfo];
	NSOperatingSystemVersion version = [pInfo operatingSystemVersion];

	v |= (uint32_t) version.majorVersion << 8;
	v |= (uint32_t) version.minorVersion;

	return v;
}

uint32_t MTY_GetPlatformNoWeb(void)
{
	return MTY_GetPlatform();
}
