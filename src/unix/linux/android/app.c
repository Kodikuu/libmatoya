// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include "app.h"

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <jni.h>
#include <android/log.h>
#include <android/input.h>

#include "keymap.h"
#include "gfx/gl-ctx.h"

static struct MTY_App {
	MTY_EventFunc event_func;
	MTY_AppFunc app_func;
	MTY_Hash *hotkey;
	MTY_Detach detach;
	uint32_t timeout;
	float scale;
	void *opaque;

	JavaVM *jvm;
	jobject obj;
	jclass cls;

	MTY_Queue *events;
	MTY_Hash *ctrls;
	MTY_Mutex *ctrl_mutex;
	MTY_Thread *log_thread;
	MTY_Thread *thread;
	MTY_Input input;
	MTY_Button long_button;
	int32_t double_tap;
	bool check_scroller;
	bool log_thread_running;
	bool should_detach;

	MTY_GFX api;
	struct gfx_ctx *gfx_ctx;
} CTX;

static const MTY_ControllerEvent APP_ZEROED_CTRL = {
	.id = 0,
	.pid = 0xCDD,
	.vid = 0xCDD,
	.numButtons = 13,
	.numValues = 7,
	.type = MTY_CTYPE_DEFAULT,
	.values = {
		[MTY_CVALUE_THUMB_LX] = {
			.usage = 0x30,
			.min = INT16_MIN,
			.max = INT16_MAX,
		},
		[MTY_CVALUE_THUMB_LY] = {
			.usage = 0x31,
			.min = INT16_MIN,
			.max = INT16_MAX,
		},
		[MTY_CVALUE_THUMB_RX] = {
			.usage = 0x32,
			.min = INT16_MIN,
			.max = INT16_MAX,
		},
		[MTY_CVALUE_THUMB_RY] = {
			.usage = 0x35,
			.min = INT16_MIN,
			.max = INT16_MAX,
		},
		[MTY_CVALUE_TRIGGER_L] = {
			.usage = 0x33,
			.min = 0,
			.max = UINT8_MAX,
		},
		[MTY_CVALUE_TRIGGER_R] = {
			.usage = 0x34,
			.min = 0,
			.max = UINT8_MAX,
		},
		[MTY_CVALUE_DPAD] = {
			.usage = 0x39,
			.data = 8,
			.min = 0,
			.max = 7,
		},
	},
};


// MTY android assumes a main entrypoint is defined

int main(int argc, char **argv);


// JNI helpers

static JNIEnv *app_jni_env(MTY_App *ctx)
{
	JNIEnv *env = NULL;

	if ((*ctx->jvm)->GetEnv(ctx->jvm, (void **) &env, JNI_VERSION_1_6) != JNI_OK)
		(*ctx->jvm)->AttachCurrentThread(ctx->jvm, &env, NULL);

	return env;
}

static void app_void_method(MTY_App *ctx, const char *name, const char *sig, ...)
{
	JNIEnv *env = app_jni_env(ctx);

	va_list args;
	va_start(args, sig);

	jmethodID mid = (*env)->GetMethodID(env, ctx->cls, name, sig);
	(*env)->CallVoidMethodV(env, ctx->obj, mid, args);

	va_end(args);
}

static int32_t app_int_method(MTY_App *ctx, const char *name, const char *sig, ...)
{
	JNIEnv *env = app_jni_env(ctx);

	va_list args;
	va_start(args, sig);

	jmethodID mid = (*env)->GetMethodID(env, ctx->cls, name, sig);
	int32_t r = (*env)->CallIntMethodV(env, ctx->obj, mid, args);

	va_end(args);

	return r;
}

static float app_float_method(MTY_App *ctx, const char *name, const char *sig, ...)
{
	JNIEnv *env = app_jni_env(ctx);

	va_list args;
	va_start(args, sig);

	jmethodID mid = (*env)->GetMethodID(env, ctx->cls, name, sig);
	float r = (*env)->CallFloatMethodV(env, ctx->obj, mid, args);

	va_end(args);

	return r;
}

static bool app_bool_method(MTY_App *ctx, const char *name, const char *sig, ...)
{
	JNIEnv *env = app_jni_env(ctx);

	va_list args;
	va_start(args, sig);

	jmethodID mid = (*env)->GetMethodID(env, ctx->cls, name, sig);
	bool r = (*env)->CallBooleanMethodV(env, ctx->obj, mid, args);

	va_end(args);

	return r;
}

static char *app_string_method(MTY_App *ctx, const char *name, const char *sig, ...)
{
	JNIEnv *env = app_jni_env(ctx);

	char *str = NULL;

	va_list args;
	va_start(args, sig);

	jmethodID mid = (*env)->GetMethodID(env, ctx->cls, name, sig);
	jstring jstr = (*env)->CallObjectMethodV(env, ctx->obj, mid, args);

	if (jstr) {
		const char *cstr = (*env)->GetStringUTFChars(env, jstr, NULL);
		str = MTY_Strdup(cstr);

		(*env)->ReleaseStringUTFChars(env, jstr, cstr);
		(*env)->DeleteLocalRef(env, jstr);
	}

	va_end(args);

	return str;
}

static jbyteArray app_jni_dup(MTY_App *ctx, const void *buf, size_t size)
{
	if (!buf || size == 0)
		return 0;

	JNIEnv *env = app_jni_env(ctx);

	jbyteArray ba = (*env)->NewByteArray(env, size);
	(*env)->SetByteArrayRegion(env, ba, 0, size, buf);

	return ba;
}

static jstring app_jni_strdup(MTY_App *ctx, const char *str)
{
	JNIEnv *env = app_jni_env(ctx);

	return (*env)->NewStringUTF(env, str);
}


// JNI entry

static void app_push_event(MTY_App *ctx, MTY_Event *evt)
{
	MTY_Event *qevt = MTY_QueueAcquireBuffer(ctx->events);
	*qevt = *evt;

	MTY_QueuePush(ctx->events, sizeof(MTY_Event));
}

static void *app_log_thread(void *opaque)
{
	MTY_App *ctx = opaque;

	// stdout & stderr redirection
	setvbuf(stdout, 0, _IOLBF, 0);
	setvbuf(stderr, 0, _IONBF, 0);

	int32_t pfd[2] = {0};
	pipe(pfd);
	dup2(pfd[1], 1);
	dup2(pfd[1], 2);

	while (ctx->log_thread_running) {
		char buf[512];
		ssize_t size = read(pfd[0], buf, 511);

		if (size <= 0)
			break;

		if (buf[size - 1] == '\n')
			size--;

		buf[size] = '\0';

		__android_log_write(ANDROID_LOG_DEBUG, "MTY", buf);
	}

	return NULL;
}

static void *app_thread(void *opaque)
{
	MTY_App *ctx = opaque;

	char *arg = "main";
	main(1, &arg);

	app_void_method(ctx, "finish", "()V");

	return NULL;
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1start(JNIEnv *env, jobject obj)
{
	(*env)->GetJavaVM(env, &CTX.jvm);

	CTX.input = MTY_INPUT_TOUCHSCREEN;
	CTX.obj = (*env)->NewGlobalRef(env, obj);

	jclass cls = (*env)->GetObjectClass(env, CTX.obj);
	CTX.cls = (*env)->NewGlobalRef(env, cls);

	mty_gfx_global_init();

	CTX.events = MTY_QueueCreate(500, sizeof(MTY_Event));
	CTX.ctrls = MTY_HashCreate(0);
	CTX.ctrl_mutex = MTY_MutexCreate();

	CTX.log_thread_running = true;
	CTX.log_thread = MTY_ThreadCreate(app_log_thread, &CTX);

	char *external = app_string_method(&CTX, "getExternalFilesDir", "()Ljava/lang/String;");
	chdir(external);
	MTY_Free(external);

	CTX.thread = MTY_ThreadCreate(app_thread, &CTX);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1stop(JNIEnv *env, jobject obj)
{
	MTY_Event evt = {0};
	evt.type = MTY_EVENT_SHUTDOWN;
	app_push_event(&CTX, &evt);

	MTY_ThreadDestroy(&CTX.thread);

	CTX.log_thread = false;
	printf("\n"); // Poke the pipe

	MTY_ThreadDestroy(&CTX.log_thread);
	MTY_MutexDestroy(&CTX.ctrl_mutex);
	MTY_HashDestroy(&CTX.ctrls, MTY_Free);
	MTY_QueueDestroy(&CTX.events);

	mty_gfx_global_destroy();

	(*env)->DeleteGlobalRef(env, CTX.obj);
	(*env)->DeleteGlobalRef(env, CTX.cls);

	memset(&CTX, 0, sizeof(MTY_App));
}


// JNI surface events

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1resize(JNIEnv *env, jobject obj,
	jint w, jint h)
{
	mty_gfx_set_dims(w, h);

	MTY_Event evt = {0};
	evt.type = MTY_EVENT_SIZE;

	app_push_event(&CTX, &evt);
}


// JNI keyboard events

JNIEXPORT jboolean JNICALL Java_group_matoya_lib_MTY_app_1key(JNIEnv *env, jobject obj,
	jboolean pressed, jint code, jstring jtext, jint mods, jboolean soft)
{
	// If trap is false, android will handle the key
	bool trap = false;

	if (pressed && jtext) {
		const char *ctext = (*env)->GetStringUTFChars(env, jtext, NULL);

		MTY_Event evt = {0};
		evt.type = MTY_EVENT_TEXT;
		snprintf(evt.text, 8, "%s", ctext);

		(*env)->ReleaseStringUTFChars(env, jtext, ctext);

		app_push_event(&CTX, &evt);
		trap = true;
	}

	// Certain soft keyboard keys come in as unique values that don't
	// correspond to EN-US keyboards, convert hem here
	if (soft)
		app_translate_soft(&code, &mods);

	// Back key has special meaning
	if (pressed && code == AKEYCODE_BACK) {
		MTY_Event evt = {0};
		evt.type = MTY_EVENT_BACK;
		app_push_event(&CTX, &evt);
		trap = true;
	}

	// Actual MTY_EVENT_KEYBOARD events generated
	if (code < (jint) APP_KEYS_MAX) {
		MTY_Key key = APP_KEYS[code];

		if (key != MTY_KEY_NONE) {
			MTY_Event evt = {0};
			evt.type = MTY_EVENT_KEYBOARD;
			evt.key.mod = app_keymods(mods);
			evt.key.pressed = pressed;

			// The soft keyboard has shift implicitly held down for certain keys
			// without ever sending the shift button, so send a shift key event
			if (soft && (evt.key.mod & MTY_MOD_SHIFT)) {
				evt.key.key = MTY_KEY_LSHIFT;
				app_push_event(&CTX, &evt);
			}

			evt.key.key = key;
			app_push_event(&CTX, &evt);
			trap = true;
		}
	}

	return trap;
}


// JNI touch events -- These are subject to "MTY_AppSetInputMode"

static void app_cancel_long_button(MTY_App *ctx, int32_t x, int32_t y)
{
	ctx->double_tap = false;

	if (ctx->long_button != MTY_BUTTON_NONE) {
		MTY_Event evt = {0};
		evt.type = MTY_EVENT_BUTTON;
		evt.button.x = x;
		evt.button.y = y;
		evt.button.pressed = false;
		evt.button.button = ctx->long_button;
		app_push_event(ctx, &evt);

		ctx->long_button = MTY_BUTTON_NONE;
	}
}

static void app_touch_mouse_button(MTY_App *ctx, int32_t x, int32_t y, MTY_Button button)
{
	app_cancel_long_button(ctx, x, y);

	MTY_Event evt = {0};
	evt.type = MTY_EVENT_BUTTON;
	evt.button.x = lrint(x);
	evt.button.y = lrint(y);
	evt.button.pressed = true;
	evt.button.button = button;
	app_push_event(ctx, &evt);

	evt.button.pressed = false;
	app_push_event(ctx, &evt);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1check_1scroller(JNIEnv *env, jobject obj,
	jboolean check)
{
	CTX.check_scroller = check;
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1unhandled_1touch(JNIEnv *env, jobject obj,
	jint action, jfloat x, jfloat y, jint fingers)
{
	CTX.should_detach = false;

	// Any time fingers come off of the screen we cancel the LONG BUTTON
	if (action == AMOTION_EVENT_ACTION_UP)
		app_cancel_long_button(&CTX, lrint(x), lrint(y));

	if (CTX.input != MTY_INPUT_TOUCHSCREEN) {
		switch (action) {
			case AMOTION_EVENT_ACTION_MOVE:
				// While a long press is in effect, move events get reported here
				// They do NOT come through in the onScroll gesture handler
				if (CTX.long_button != MTY_BUTTON_NONE) {
					MTY_Event evt = {0};
					evt.type = MTY_EVENT_MOTION;
					evt.motion.x = lrint(x);
					evt.motion.y = lrint(y);
					app_push_event(&CTX, &evt);
				}
				break;
			case AMOTION_EVENT_ACTION_POINTER_DOWN:
				// Taps with two fingers need to be handled manually
				// They are not detected by the gesture recognizer
				CTX.double_tap = fingers > 1;
				break;
			case AMOTION_EVENT_ACTION_POINTER_UP:
				if (CTX.double_tap)
					app_touch_mouse_button(&CTX, x, y, MTY_BUTTON_RIGHT);

				CTX.double_tap = false;
				break;
		}
	}
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1single_1tap_1up(JNIEnv *env, jobject obj,
	jfloat x, jfloat y)
{
	CTX.should_detach = false;

	app_touch_mouse_button(&CTX, x, y, MTY_BUTTON_LEFT);
}

JNIEXPORT jboolean JNICALL Java_group_matoya_lib_MTY_app_1long_1press(JNIEnv *env, jobject obj,
	jfloat x, jfloat y)
{
	CTX.should_detach = false;

	app_cancel_long_button(&CTX, lrint(x), lrint(y));

	// Long press in touchscreen mode is a simple right click
	if (CTX.input == MTY_INPUT_TOUCHSCREEN) {
		MTY_Event evt = {0};
		evt.type = MTY_EVENT_BUTTON;
		evt.button.x = lrint(x);
		evt.button.y = lrint(y);
		evt.button.pressed = true;
		evt.button.button = MTY_BUTTON_RIGHT;
		app_push_event(&CTX, &evt);

		evt.button.pressed = false;
		app_push_event(&CTX, &evt);

	// In trackpad mode, begin a mouse down and set the LONG_BUTTON state
	} else {
		CTX.long_button = MTY_BUTTON_LEFT;

		MTY_Event evt = {0};
		evt.type = MTY_EVENT_BUTTON;
		evt.button.x = lrint(x);
		evt.button.y = lrint(y);
		evt.button.pressed = true;
		evt.button.button = CTX.long_button;
		app_push_event(&CTX, &evt);

		return true;
	}

	return false;
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1scroll(JNIEnv *env, jobject obj,
	jfloat abs_x, jfloat abs_y, jfloat x, jfloat y, jint fingers)
{
	CTX.should_detach = false;

	app_cancel_long_button(&CTX, lrint(abs_x), lrint(abs_y));

	// Single finger scrolling in touchscreen mode OR two finger scrolling in
	// trackpad mode moves to the touch location and produces a scroll event
	if (CTX.input == MTY_INPUT_TOUCHSCREEN ||
		(CTX.long_button == MTY_BUTTON_NONE && fingers > 1))
	{
		MTY_Event evt = {0};

		// Negative init values mean "don't move the cursor"
		if (abs_x > 0.0f || abs_y > 0.0f) {
			evt.type = MTY_EVENT_MOTION;
			evt.motion.x = lrint(abs_x);
			evt.motion.y = lrint(abs_y);
			app_push_event(&CTX, &evt);
		}

		evt.type = MTY_EVENT_SCROLL;
		evt.scroll.pixels = true;
		evt.scroll.y = -lrint(y);
		evt.scroll.x = 0;
		app_push_event(&CTX, &evt);

	// While single finger scrolling in trackpad mode, convert to mouse motion
	} else if (abs_x > 0.0f || abs_y > 0.0f) {
		MTY_Event evt = {0};
		evt.type = MTY_EVENT_MOTION;
		evt.motion.x = lrint(abs_x);
		evt.motion.y = lrint(abs_y);
		app_push_event(&CTX, &evt);
	}
}


// JNI mouse events

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1mouse_1motion(JNIEnv *env, jobject obj,
	jboolean relative, jfloat x, jfloat y)
{
	CTX.should_detach = true;

	app_cancel_long_button(&CTX, lrint(x), lrint(y));

	MTY_Event evt = {0};
	evt.type = MTY_EVENT_MOTION;
	evt.motion.relative = relative;
	evt.motion.x = lrint(x);
	evt.motion.y = lrint(y);
	app_push_event(&CTX, &evt);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1generic_1scroll(JNIEnv *env, jobject obj,
	jfloat x, jfloat y)
{
	CTX.should_detach = true;

	MTY_Event evt = {0};
	evt.type = MTY_EVENT_SCROLL;
	evt.scroll.x = x > 0.0f ? 120 : x < 0.0f ? -120 : 0;
	evt.scroll.y = y > 0.0f ? 120 : y < 0.0f ? -120 : 0;
	app_push_event(&CTX, &evt);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1mouse_1button(JNIEnv *env, jobject obj,
	jboolean pressed, jint button, jfloat x, jfloat y)
{
	CTX.should_detach = true;

	app_cancel_long_button(&CTX, lrint(x), lrint(y));

	MTY_Event evt = {0};
	evt.type = MTY_EVENT_BUTTON;
	evt.button.x = lrint(x);
	evt.button.y = lrint(y);
	evt.button.pressed = pressed;

	switch (button) {
		case AMOTION_EVENT_BUTTON_PRIMARY:
			evt.button.button = MTY_BUTTON_LEFT;
			break;
		case AMOTION_EVENT_BUTTON_SECONDARY:
			evt.button.button = MTY_BUTTON_RIGHT;
			break;
		case AMOTION_EVENT_BUTTON_TERTIARY:
			evt.button.button = MTY_BUTTON_MIDDLE;
			break;
		case AMOTION_EVENT_BUTTON_BACK:
			evt.button.button = MTY_BUTTON_X1;
			break;
		case AMOTION_EVENT_BUTTON_FORWARD:
			evt.button.button = MTY_BUTTON_X2;
			break;
	}

	if (evt.button.button != MTY_BUTTON_NONE)
		app_push_event(&CTX, &evt);
}


// JNI controller events

static MTY_ControllerEvent *app_get_controller(MTY_App *ctx, int32_t deviceId)
{
	MTY_ControllerEvent *c = MTY_HashGetInt(ctx->ctrls, deviceId);
	if (!c) {
		int32_t ids = app_int_method(ctx, "getHardwareIds", "(I)I", deviceId);

		c = MTY_Alloc(1, sizeof(MTY_ControllerEvent));
		*c = APP_ZEROED_CTRL;

		c->id = deviceId;
		c->vid = ids >> 16;
		c->pid = ids & 0xFFFF;

		MTY_HashSetInt(ctx->ctrls, deviceId, c);

		MTY_Event evt = {0};
		evt.type = MTY_EVENT_CONNECT;
		evt.controller = *c;
		app_push_event(ctx, &evt);
	}

	return c;
}

static void app_remove_controller(MTY_App *ctx, int32_t deviceId)
{
	MTY_ControllerEvent *c = MTY_HashGetInt(ctx->ctrls, deviceId);
	if (c) {
		*c = APP_ZEROED_CTRL;

		c->id = deviceId;

		MTY_Event evt = {0};
		evt.type = MTY_EVENT_DISCONNECT;
		evt.controller = *c;
		app_push_event(ctx, &evt);

		MTY_Free(MTY_HashPopInt(ctx->ctrls, deviceId));
	}
}

static void app_push_controller_event(MTY_App *ctx, const MTY_ControllerEvent *c)
{
	MTY_Event evt = {0};
	evt.type = MTY_EVENT_CONTROLLER;
	evt.controller = *c;
	app_push_event(ctx, &evt);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1button(JNIEnv *env, jobject obj,
	jint deviceId, jboolean pressed, jint button)
{
	MTY_MutexLock(CTX.ctrl_mutex);

	MTY_ControllerEvent *c = app_get_controller(&CTX, deviceId);

	switch (button) {
		case AKEYCODE_DPAD_CENTER: c->buttons[MTY_CBUTTON_A] = pressed; break;
		case AKEYCODE_BUTTON_A: c->buttons[MTY_CBUTTON_A] = pressed; break;
		case AKEYCODE_BUTTON_B: c->buttons[MTY_CBUTTON_B] = pressed; break;
		case AKEYCODE_BUTTON_X: c->buttons[MTY_CBUTTON_X] = pressed; break;
		case AKEYCODE_BUTTON_Y: c->buttons[MTY_CBUTTON_Y] = pressed; break;
		case AKEYCODE_BUTTON_L1: c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = pressed; break;
		case AKEYCODE_BUTTON_R1: c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = pressed; break;
		case AKEYCODE_BUTTON_L2: c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = pressed; break;
		case AKEYCODE_BUTTON_R2: c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = pressed; break;
		case AKEYCODE_BUTTON_THUMBL: c->buttons[MTY_CBUTTON_LEFT_THUMB] = pressed; break;
		case AKEYCODE_BUTTON_THUMBR: c->buttons[MTY_CBUTTON_RIGHT_THUMB] = pressed; break;
		case AKEYCODE_BUTTON_START: c->buttons[MTY_CBUTTON_START] = pressed; break;
		case AKEYCODE_BUTTON_SELECT: c->buttons[MTY_CBUTTON_BACK] = pressed; break;
		case AKEYCODE_BUTTON_MODE: c->buttons[MTY_CBUTTON_GUIDE] = pressed; break;

		case AKEYCODE_DPAD_UP:
		case AKEYCODE_DPAD_RIGHT:
		case AKEYCODE_DPAD_DOWN:
		case AKEYCODE_DPAD_LEFT:
		case AKEYCODE_DPAD_UP_LEFT:
		case AKEYCODE_DPAD_DOWN_LEFT:
		case AKEYCODE_DPAD_UP_RIGHT:
		case AKEYCODE_DPAD_DOWN_RIGHT: {
			bool up = pressed && (button == AKEYCODE_DPAD_UP || button == AKEYCODE_DPAD_UP_LEFT || button == AKEYCODE_DPAD_UP_RIGHT);
			bool down = pressed && (button == AKEYCODE_DPAD_DOWN || button == AKEYCODE_DPAD_DOWN_LEFT || button == AKEYCODE_DPAD_DOWN_RIGHT);
			bool left = pressed && (button == AKEYCODE_DPAD_LEFT || button == AKEYCODE_DPAD_UP_LEFT || button == AKEYCODE_DPAD_DOWN_LEFT);
			bool right = pressed && (button == AKEYCODE_DPAD_RIGHT || button == AKEYCODE_DPAD_UP_RIGHT || button == AKEYCODE_DPAD_DOWN_RIGHT);

			c->values[MTY_CVALUE_DPAD].data = (up && right) ? 1 : (right && down) ? 3 :
				(down && left) ? 5 : (left && up) ? 7 : up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;
			break;
		}
	}

	app_push_controller_event(&CTX, c);

	MTY_MutexUnlock(CTX.ctrl_mutex);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1axis(JNIEnv *env, jobject obj,
	jint deviceId, jfloat hatX, jfloat hatY, jfloat lX, jfloat lY, jfloat rX, jfloat rY,
	jfloat lT, jfloat rT, jfloat lTalt, jfloat rTalt)
{
	MTY_MutexLock(CTX.ctrl_mutex);

	MTY_ControllerEvent *c = app_get_controller(&CTX, deviceId);

	c->values[MTY_CVALUE_THUMB_LX].data = lrint(lX * (float) (lX < 0.0f ? abs(INT16_MIN) : INT16_MAX));
	c->values[MTY_CVALUE_THUMB_LY].data = lrint(-lY * (float) (-lY < 0.0f ? abs(INT16_MIN) : INT16_MAX));
	c->values[MTY_CVALUE_THUMB_RX].data = lrint(rX * (float) (rX < 0.0f ? abs(INT16_MIN) : INT16_MAX));
	c->values[MTY_CVALUE_THUMB_RY].data = lrint(-rY * (float) (-rY < 0.0f ? abs(INT16_MIN) : INT16_MAX));

	c->values[MTY_CVALUE_TRIGGER_L].data = lrint(lT * (float) UINT8_MAX);
	c->values[MTY_CVALUE_TRIGGER_R].data = lrint(rT * (float) UINT8_MAX);

	// Xbox Series X hack
	if (c->vid == 0x045E && c->pid == 0x0B13) {
		if (c->values[MTY_CVALUE_TRIGGER_L].data == 0)
			c->values[MTY_CVALUE_TRIGGER_L].data = lrint(lTalt * (float) UINT8_MAX);

		if (c->values[MTY_CVALUE_TRIGGER_R].data == 0)
			c->values[MTY_CVALUE_TRIGGER_R].data = lrint(rTalt * (float) UINT8_MAX);
	}

	bool up = hatY == -1.0f;
	bool down = hatY == 1.0f;
	bool left = hatX == -1.0f;
	bool right = hatX == 1.0f;

	c->values[MTY_CVALUE_DPAD].data = (up && right) ? 1 : (right && down) ? 3 :
		(down && left) ? 5 : (left && up) ? 7 : up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;

	app_push_controller_event(&CTX, c);

	MTY_MutexUnlock(CTX.ctrl_mutex);
}

JNIEXPORT void JNICALL Java_group_matoya_lib_MTY_app_1unplug(JNIEnv *env, jobject obj,
	jint deviceId)
{
	MTY_MutexLock(CTX.ctrl_mutex);

	app_remove_controller(&CTX, deviceId);

	MTY_MutexUnlock(CTX.ctrl_mutex);
}


// App

void MTY_HotkeyToString(MTY_Mod mod, MTY_Key key, char *str, size_t len)
{
	memset(str, 0, len);

	MTY_Strcat(str, len, (mod & MTY_MOD_WIN) ? "Super+" : "");
	MTY_Strcat(str, len, (mod & MTY_MOD_CTRL) ? "Ctrl+" : "");
	MTY_Strcat(str, len, (mod & MTY_MOD_ALT) ? "Alt+" : "");
	MTY_Strcat(str, len, (mod & MTY_MOD_SHIFT) ? "Shift+" : "");

	if (key != MTY_KEY_NONE) {
		for (int32_t x = 0; x < (int32_t) APP_KEYS_MAX; x++) {
			if (key == APP_KEYS[x]) {
				char *ctext = app_string_method(&CTX, "getKey", "(I)Ljava/lang/String;", x);
				if (ctext) {
					MTY_Strcat(str, len, ctext);
					MTY_Free(ctext);
				}

				break;
			}
		}
	}
}

uint32_t MTY_AppGetHotkey(MTY_App *ctx, MTY_Scope scope, MTY_Mod mod, MTY_Key key)
{
	mod &= 0xFF;

	return (uint32_t) (uintptr_t) MTY_HashGetInt(ctx->hotkey, (mod << 16) | key);
}

void MTY_AppSetHotkey(MTY_App *ctx, MTY_Scope scope, MTY_Mod mod, MTY_Key key, uint32_t id)
{
	mod &= 0xFF;
	MTY_HashSetInt(ctx->hotkey, (mod << 16) | key, (void *) (uintptr_t) id);
}

void MTY_AppRemoveHotkeys(MTY_App *ctx, MTY_Scope scope)
{
	MTY_HashDestroy(&ctx->hotkey, NULL);
	ctx->hotkey = MTY_HashCreate(0);
}

char *MTY_AppGetClipboard(MTY_App *app)
{
	return app_string_method(app, "getClipboard", "()Ljava/lang/String;");
}

void MTY_AppSetClipboard(MTY_App *app, const char *text)
{
	app_void_method(app, "setClipboard", "(Ljava/lang/String;)V", app_jni_strdup(app, text));
}

static float app_get_scale(MTY_App *ctx)
{
	return app_float_method(ctx, "getDisplayDensity", "()F") * 0.85f;
}

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_EventFunc eventFunc, void *opaque)
{
	MTY_App *ctx = &CTX;

	ctx->app_func = appFunc;
	ctx->event_func = eventFunc;
	ctx->opaque = opaque;
	ctx->hotkey = MTY_HashCreate(0);

	ctx->scale = app_get_scale(ctx);

	return ctx;
}

void MTY_AppDestroy(MTY_App **app)
{
	if (!app || !*app)
		return;

	MTY_App *ctx = *app;

	MTY_HashDestroy(&ctx->hotkey, NULL);
	*app = NULL;
}

static bool app_check_focus(MTY_App *ctx, bool was_ready)
{
	MTY_Event evt = {0};
	evt.type = MTY_EVENT_FOCUS;
	evt.focus = mty_gfx_is_ready();

	if ((!was_ready && evt.focus) || (was_ready && !evt.focus))
		ctx->event_func(&evt, ctx->opaque);

	return evt.focus;
}

static void app_kb_to_hotkey(MTY_App *app, MTY_Event *evt)
{
	MTY_Mod mod = evt->key.mod & 0xFF;
	uint32_t hotkey = (uint32_t) (uintptr_t) MTY_HashGetInt(app->hotkey, (mod << 16) | evt->key.key);

	if (hotkey != 0) {
		if (evt->key.pressed) {
			evt->type = MTY_EVENT_HOTKEY;
			evt->hotkey = hotkey;

		} else {
			evt->type = MTY_EVENT_NONE;
		}
	}
}

void MTY_AppRun(MTY_App *ctx)
{
	for (bool cont = true, was_ready = false; cont;) {
		for (MTY_Event *evt; MTY_QueuePop(ctx->events, 0, (void **) &evt, NULL);) {
			if (evt->type == MTY_EVENT_KEYBOARD)
				app_kb_to_hotkey(ctx, evt);

			ctx->event_func(evt, ctx->opaque);
			MTY_QueueReleaseBuffer(ctx->events);
		}

		// Generate MTY_EVENT_FOCUS events
		was_ready = app_check_focus(ctx, was_ready);

		// Generates scroll events after a fling has taken place
		// Prevent JNI calls if there's no fling in progress
		if (ctx->check_scroller)
			app_void_method(ctx, "checkScroller", "()V");

		cont = ctx->app_func(ctx->opaque);

		if (ctx->timeout > 0)
			MTY_Sleep(ctx->timeout);
	}
}

void MTY_AppSetTimeout(MTY_App *ctx, uint32_t timeout)
{
	ctx->timeout = timeout;
}

void MTY_AppEnableScreenSaver(MTY_App *app, bool enable)
{
	app_void_method(app, "enableScreenSaver", "(Z)V", enable);
}

bool MTY_AppIsActive(MTY_App *ctx)
{
	return mty_gfx_is_ready();
}

void MTY_AppShowSoftKeyboard(MTY_App *app, bool show)
{
	app_void_method(app, "showKeyboard", "(Z)V", show);
}

bool MTY_AppSoftKeyboardIsShowing(MTY_App *app)
{
	return app_bool_method(app, "keyboardIsShowing", "()Z");
}

MTY_Orientation MTY_AppGetOrientation(MTY_App *ctx)
{
	return app_int_method(ctx, "getOrientation", "()I");
}

void MTY_AppSetOrientation(MTY_App *app, MTY_Orientation orientation)
{
	app_void_method(app, "setOrientation", "(I)V", orientation);
}

void MTY_AppSetPNGCursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
	app_void_method(app, "setCursor", "([BFF)V", app_jni_dup(app, image, size), (jfloat) hotX, (jfloat) hotY);
}

bool MTY_AppCanWarpCursor(MTY_App *ctx)
{
	return false;
}

void MTY_AppShowCursor(MTY_App *ctx, bool show)
{
	app_void_method(ctx, "showCursor", "(Z)V", show);
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
	app_void_method(app, "useDefaultCursor", "(Z)V", useDefault);
}

void MTY_AppSetRelativeMouse(MTY_App *app, bool relative)
{
	app_void_method(app, "setRelativeMouse", "(Z)V", relative);
}

bool MTY_AppGetRelativeMouse(MTY_App *app)
{
	return app_bool_method(app, "getRelativeMouse", "()Z");
}

void MTY_AppDetach(MTY_App *app, MTY_Detach type)
{
	app->detach = type;
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
	// When touch events are receved, don't respect detach

	return app->should_detach ? app->detach : MTY_DETACH_NONE;
}

MTY_Input MTY_AppGetInputMode(MTY_App *ctx)
{
	return ctx->input;
}

void MTY_AppSetInputMode(MTY_App *ctx, MTY_Input mode)
{
	ctx->input = mode;
}


// Window

MTY_Window MTY_WindowCreate(MTY_App *app, const char *title, const MTY_WindowDesc *desc)
{
	return 0;
}

void MTY_WindowSetFullscreen(MTY_App *app, MTY_Window window, bool fullscreen)
{
	app_void_method(app, "enableFullscreen", "(Z)V", fullscreen);
}

bool MTY_WindowIsFullscreen(MTY_App *app, MTY_Window window)
{
	return app_bool_method(app, "isFullscreen", "()Z");
}

bool MTY_WindowGetScreenSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	mty_gfx_size(width, height);

	return true;
}

float MTY_WindowGetScale(MTY_App *app, MTY_Window window)
{
	return app->scale;
}

bool MTY_WindowIsVisible(MTY_App *app, MTY_Window window)
{
	return mty_gfx_is_ready();
}

bool MTY_WindowIsActive(MTY_App *app, MTY_Window window)
{
	return mty_gfx_is_ready();
}

bool MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	MTY_WindowGetScreenSize(app, window, width, height);

	int32_t kb_height = app_int_method(app, "keyboardHeight", "()I");

	if (kb_height == -1)
		kb_height = 0;

	mty_gfx_set_kb_height(kb_height);
	*height -= kb_height;

	return true;
}

bool MTY_WindowExists(MTY_App *app, MTY_Window window)
{
	return true;
}

MTY_GFXState MTY_WindowGFXState(MTY_App *app, MTY_Window window)
{
	return mty_gfx_state();
}


// Window Private

void mty_window_set_gfx(MTY_App *app, MTY_Window window, MTY_GFX api, struct gfx_ctx *gfx_ctx)
{
	app->api = api;
	app->gfx_ctx = gfx_ctx;
}

MTY_GFX mty_window_get_gfx(MTY_App *app, MTY_Window window, struct gfx_ctx **gfx_ctx)
{
	if (gfx_ctx)
		*gfx_ctx = app->gfx_ctx;

	return app->api;
}

void *mty_window_get_native(MTY_App *app, MTY_Window window)
{
	return (void *) (uintptr_t) CTX.obj;
}


// Misc

void MTY_MessageBox(const char *title, const char *fmt, ...)
{
}

void MTY_SetAppID(const char *id)
{
}

void *MTY_JNIEnv(void)
{
	return app_jni_env(&CTX);
}


// Unimplemented

void MTY_AppControllerRumble(MTY_App *app, uint32_t id, uint16_t low, uint16_t high)
{
}

bool MTY_AppKeyboardIsGrabbed(MTY_App *ctx)
{
	return false;
}

void MTY_AppGrabKeyboard(MTY_App *app, bool grab)
{
}

void MTY_WindowSetTitle(MTY_App *app, MTY_Window window, const char *title)
{
}

void MTY_AppActivate(MTY_App *app, bool active)
{
}

void MTY_WindowActivate(MTY_App *app, MTY_Window window, bool active)
{
}

void MTY_WindowWarpCursor(MTY_App *app, MTY_Window window, uint32_t x, uint32_t y)
{
}

bool MTY_AppMouseIsGrabbed(MTY_App *ctx)
{
	return false;
}

void MTY_AppGrabMouse(MTY_App *app, bool grab)
{
}

void MTY_WindowDestroy(MTY_App *app, MTY_Window window)
{
}

void *MTY_GLGetProcAddress(const char *name)
{
	return NULL;
}

void MTY_AppSetTray(MTY_App *app, const char *tooltip, const MTY_MenuItem *items, uint32_t len)
{
}

void MTY_AppRemoveTray(MTY_App *app)
{
}

void MTY_AppNotification(MTY_App *app, const char *title, const char *msg)
{
}

void MTY_AppEnableGlobalHotkeys(MTY_App *app, bool enable)
{
}

bool MTY_AppPenIsEnabled(MTY_App *ctx)
{
	return false;
}

void MTY_AppEnablePen(MTY_App *ctx, bool enable)
{
}
