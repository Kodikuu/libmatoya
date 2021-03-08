// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include "app.h"
#include "jnih.h"

void MTY_ProtocolHandler(const char *uri, void *token)
{
	jobject obj = mty_window_get_native(NULL, 0);

	if (obj) {
		JNIEnv *env = MTY_JNIEnv();

		jstring juri = mty_jni_strdup(env, uri);
		mty_jni_void(env, obj, "openURI", "(Ljava/lang/String;)V", juri);
		mty_jni_free(env, juri);
	}
}

uint32_t MTY_GetPlatform(void)
{
	uint32_t v = MTY_OS_ANDROID;

	int32_t level = android_get_device_api_level();

	v |= (uint32_t) level << 8;

	return v;
}

uint32_t MTY_GetPlatformNoWeb(void)
{
	return MTY_GetPlatform();
}
