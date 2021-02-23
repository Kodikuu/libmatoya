// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "app.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include <unistd.h>

#include "x-dl.h"
#include "wsize.h"
#include "keymap.h"
#include "hid/hid.h"
#include "hid/driver.h"

struct window {
	Window window;
	MTY_Window index;
	XIC ic;
	MTY_GFX api;
	struct gfx_ctx *gfx_ctx;
	struct xinfo info;
};

struct MTY_App {
	Display *display;
	Cursor empty_cursor;
	Cursor custom_cursor;
	Cursor cursor;
	bool default_cursor;
	bool hide_cursor;
	char *class_name;
	MTY_Detach detach;
	XVisualInfo *vis;
	Atom wm_close;
	Atom wm_ping;
	Window sel_owner;
	XIM im;

	MTY_MsgFunc msg_func;
	MTY_AppFunc app_func;
	MTY_Hash *hotkey;
	MTY_Mutex *mutex;
	struct hid *hid;
	struct window *windows[MTY_WINDOW_MAX];
	uint32_t timeout;
	int64_t suspend_ts;
	bool relative;
	bool suspend_ss;
	bool mgrab;
	bool kbgrab;
	void *opaque;
	char *clip;
	float scale;

	uint64_t state;
	uint64_t prev_state;
};


// Helpers, DL

static void __attribute__((destructor)) app_global_destroy(void)
{
	x_dl_global_destroy();
}

static struct window *app_get_window(MTY_App *ctx, MTY_Window window)
{
	return window < 0 ? NULL : ctx->windows[window];
}

static MTY_Window app_find_open_window(MTY_App *ctx)
{
	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++)
		if (!ctx->windows[x])
			return x;

	return -1;
}

static MTY_Window app_find_window(MTY_App *ctx, Window xwindow)
{
	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++) {
		struct window *win = app_get_window(ctx, x);

		if (win->window == xwindow)
			return x;
	}

	return 0;
}

static struct window *app_get_active_window(MTY_App *ctx)
{
	Window w = 0;
	int32_t revert = 0;
	XGetInputFocus(ctx->display, &w, &revert);

	for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++) {
		struct window *win = app_get_window(ctx, x);

		if (win && win->window == w)
			return win;
	}

	return NULL;
}


// Hotkeys

static MTY_Atomic32 APP_GLOCK;
static char APP_KEYS[MTY_KEY_MAX][16];

static void app_hotkey_init(void)
{
	if (MTY_Atomic32Get(&APP_GLOCK) == 0) {
		MTY_GlobalLock(&APP_GLOCK);

		Display *display = XOpenDisplay(NULL);
		XIM xim = XOpenIM(display, 0, 0, 0);
		XIC xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, NULL);

		for (MTY_Key sc = 0; sc < MTY_KEY_MAX; sc++) {
			unsigned int keycode = APP_KEY_MAP[sc];
			if (keycode == 0)
				continue;

			XKeyPressedEvent evt = {0};
			evt.type = KeyPress;
			evt.display = display;
			evt.keycode = keycode;

			KeySym sym = XLookupKeysym(&evt, 0);
			if (sym == NoSymbol)
				continue;

			KeySym lsym = sym, usym = sym;
			XConvertCase(sym, &lsym, &usym);

			char utf8_str[16] = {0};
			bool lookup_ok = false;

			// FIXME symbols >= 0x80 seem to crash Xutf8LookupString -- why?
			if (sym < 0x80) {
				evt.state = sym != usym ? ShiftMask : 0;

				KeySym ignore;
				Status status;
				lookup_ok = Xutf8LookupString(xic, &evt, utf8_str, 16, &ignore, &status) > 0;
			}

			if (!lookup_ok) {
				const char *sym_str = XKeysymToString(usym);
				if (sym_str)
					snprintf(utf8_str, 16, "%s", sym_str);
			}

			snprintf(APP_KEYS[sc], 16, "%s", utf8_str);
		}

		XDestroyIC(xic);
		XCloseIM(xim);
		XCloseDisplay(display);

		MTY_GlobalUnlock(&APP_GLOCK);
	}
}

void MTY_AppHotkeyToString(MTY_Mod mod, MTY_Key key, char *str, size_t len)
{
	memset(str, 0, len);

	if (!x_dl_global_init())
		return;

	app_hotkey_init();

	MTY_Strcat(str, len, (mod & MTY_MOD_WIN) ? "Super+" : "");
	MTY_Strcat(str, len, (mod & MTY_MOD_CTRL) ? "Ctrl+" : "");
	MTY_Strcat(str, len, (mod & MTY_MOD_ALT) ? "Alt+" : "");
	MTY_Strcat(str, len, (mod & MTY_MOD_SHIFT) ? "Shift+" : "");

	MTY_Strcat(str, len, APP_KEYS[key]);
}

void MTY_AppSetHotkey(MTY_App *ctx, MTY_Hotkey mode, MTY_Mod mod, MTY_Key key, uint32_t id)
{
	mod &= 0xFF;
	MTY_HashSetInt(ctx->hotkey, (mod << 16) | key, (void *) (uintptr_t) id);
}

void MTY_AppRemoveHotkeys(MTY_App *ctx, MTY_Hotkey mode)
{
	MTY_HashDestroy(&ctx->hotkey, NULL);
	ctx->hotkey = MTY_HashCreate(0);
}


// Clipboard (selections)

static void app_handle_selection_request(MTY_App *ctx, const XEvent *event)
{
	// Unlike other OSs, X11 receives a request from other applications
	// to set a window property (Atom)

	struct window *win0 = app_get_window(ctx, 0);
	if (!win0)
		return;

	const XSelectionRequestEvent *req = &event->xselectionrequest;

	XEvent snd = {0};
	snd.type = SelectionNotify;

	XSelectionEvent *res = &snd.xselection;
	res->selection = req->selection;
	res->requestor = req->requestor;
	res->time = req->time;

	unsigned long bytes, overflow;
	unsigned char *data = NULL;
	int format = 0;

	Atom targets = XInternAtom(ctx->display, "TARGETS", False);
	Atom mty_clip = XInternAtom(ctx->display, "MTY_CLIPBOARD", False);

	// Get the utf8 clipboard buffer associated with window 0
	if (XGetWindowProperty(ctx->display, win0->window, mty_clip, 0, INT_MAX / 4, False, req->target,
		&res->target, &format, &bytes, &overflow, &data) == Success)
	{
		// Requestor wants the data, if the target (format) matches our buffer, set it
		if (req->target == res->target) {
			XChangeProperty(ctx->display, req->requestor, req->property, res->target,
				format, PropModeReplace, data, bytes);

			res->property = req->property;

		// Requestor is querying which targets (formats) are available (the TARGETS atom)
		} else if (req->target == targets) {
			Atom formats[] = {targets, res->target};
			XChangeProperty(ctx->display, req->requestor, req->property, XA_ATOM, 32, PropModeReplace,
				(unsigned char *) formats, 2);

			res->property = req->property;
			res->target = targets;
		}

		XFree(data);
	}

	// Send the response event
	XSendEvent(ctx->display, req->requestor, False, 0, &snd);
	XSync(ctx->display, False);
}

static void app_handle_selection_notify(MTY_App *ctx, const XSelectionEvent *res)
{
	struct window *win0 = app_get_window(ctx, 0);
	if (!win0)
		return;

	Atom format = XInternAtom(ctx->display, "UTF8_STRING", False);
	Atom mty_clip = XInternAtom(ctx->display, "MTY_CLIPBOARD", False);

	// Unhandled format or bad conversion
	if (!res->property || res->target != format)
		return;

	unsigned long overflow, bytes;
	unsigned char *src = NULL;
	Atom res_type;
	int res_format;

	if (XGetWindowProperty(ctx->display, win0->window, mty_clip, 0, INT_MAX / 4, False,
		format, &res_type, &res_format, &bytes, &overflow, &src) == Success)
	{
		MTY_MutexLock(ctx->mutex);

		MTY_Free(ctx->clip);
		ctx->clip = MTY_Alloc(bytes + 1, 1);
		memcpy(ctx->clip, src, bytes);

		MTY_MutexUnlock(ctx->mutex);

		// FIXME this is a questionable technique: will it interfere with other applications?
		// We take back the selection so that we can be notified the next time a different app takes it
		XSetSelectionOwner(ctx->display, XInternAtom(ctx->display, "CLIPBOARD", False), win0->window, CurrentTime);

		MTY_Msg msg = {0};
		msg.type = MTY_MSG_CLIPBOARD;
		ctx->msg_func(&msg, ctx->opaque);
	}
}

static void app_poll_clipboard(MTY_App *ctx)
{
	Atom clip = XInternAtom(ctx->display, "CLIPBOARD", False);

	Window sel_owner = XGetSelectionOwner(ctx->display, clip);
	if (sel_owner != ctx->sel_owner) {
		struct window *win0 = app_get_window(ctx, 0);

		if (win0 && sel_owner != win0->window) {
			Atom format = XInternAtom(ctx->display, "UTF8_STRING", False);
			Atom mty_clip = XInternAtom(ctx->display, "MTY_CLIPBOARD", False);

			// This will send the request out to the owner
			XConvertSelection(ctx->display, clip, format, mty_clip, win0->window, CurrentTime);
		}

		ctx->sel_owner = sel_owner;
	}
}

char *MTY_AppGetClipboard(MTY_App *app)
{
	MTY_MutexLock(app->mutex);

	char *str = MTY_Strdup(app->clip);

	MTY_MutexUnlock(app->mutex);

	return str;
}

void MTY_AppSetClipboard(MTY_App *app, const char *text)
{
	struct window *ctx = app_get_window(app, 0);
	if (!ctx)
		return;

	Atom clip = XInternAtom(app->display, "CLIPBOARD", False);
	Atom mty_clip = XInternAtom(app->display, "MTY_CLIPBOARD", False);
	Atom format = XInternAtom(app->display, "UTF8_STRING", False);

	XChangeProperty(app->display, ctx->window, mty_clip,
		format, 8, PropModeReplace, (const unsigned char *) text, strlen(text));

	if (XGetSelectionOwner(app->display, clip) != ctx->window)
		XSetSelectionOwner(app->display, clip, ctx->window, CurrentTime);

	if (XGetSelectionOwner(app->display, XA_PRIMARY) != ctx->window)
		XSetSelectionOwner(app->display, XA_PRIMARY, ctx->window, CurrentTime);
}


// Cursor/grab state

static void app_apply_keyboard_grab(MTY_App *app, struct window *win)
{
	if (win && app->kbgrab && app->detach == MTY_DETACH_NONE) {
		XGrabKeyboard(app->display, win->window, False, GrabModeAsync, GrabModeAsync, CurrentTime);

	} else {
		XUngrabKeyboard(app->display, CurrentTime);
	}
}

static void app_apply_mouse_grab(MTY_App *app, struct window *win)
{
	if (win && // One of our windows is the focus window
		((app->relative && app->detach != MTY_DETACH_FULL) || // In relative mode and not fully detached
		(app->mgrab && app->detach == MTY_DETACH_NONE))) // Mouse grab active and not detached
	{
		XGrabPointer(app->display, win->window, False,
			ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask,
			GrabModeAsync, GrabModeAsync, win->window, None, CurrentTime);

	} else {
		// FIXME This should also warp to the stored coordinates when SetRelative was called
		XUngrabPointer(app->display, CurrentTime);
	}
}

static void app_apply_cursor(MTY_App *app, bool focus)
{
	Cursor cur = focus && (app->hide_cursor || (app->relative && app->detach == MTY_DETACH_NONE)) ?
		app->empty_cursor : app->custom_cursor && !app->default_cursor ?
		app->custom_cursor : None;

	if (cur != app->cursor) {
		for (MTY_Window x = 0; x < MTY_WINDOW_MAX; x++) {
			struct window *win = app_get_window(app, x);

			if (win)
				XDefineCursor(app->display, win->window, cur);
		}

		app->cursor = cur;
	}
}


// Cursor

static Cursor app_png_cursor(Display *display, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
	Cursor cursor = None;
	XcursorImage *ximage = NULL;

	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t *rgba = MTY_DecompressImage(image, size, &width, &height);
	if (!rgba)
		goto except;

	ximage = XcursorImageCreate(width, height);
	if (!ximage)
		goto except;

	ximage->xhot = hotX;
	ximage->yhot = hotY;
	ximage->delay = 0;

	for (uint32_t i = 0; i < width * height; i++) {
		uint32_t p = rgba[i];
		uint32_t a = (p & 0x00FF0000) >> 16;
		uint32_t b = (p & 0x000000FF) << 16;;

		ximage->pixels[i] = (p & 0xFF00FF00) | a | b;
	}

	cursor = XcursorImageLoadCursor(display, ximage);

	except:

	if (ximage)
		XcursorImageDestroy(ximage);

	MTY_Free(rgba);

	return cursor;
}

void MTY_AppSetPNGCursor(MTY_App *app, const void *image, size_t size, uint32_t hotX, uint32_t hotY)
{
	Cursor prev = app->custom_cursor;
	app->custom_cursor = None;

	if (image && size > 0)
		app->custom_cursor = app_png_cursor(app->display, image, size, hotX, hotY);

	if (prev)
		XFreeCursor(app->display, prev);

	app->state++;
}

void MTY_AppUseDefaultCursor(MTY_App *app, bool useDefault)
{
	if (app->default_cursor != useDefault) {
		app->default_cursor = useDefault;
		app->state++;
	}
}

void MTY_AppShowCursor(MTY_App *ctx, bool show)
{
	if (ctx->hide_cursor == show) {
		ctx->hide_cursor = !show;
		ctx->state++;
	}
}

bool MTY_AppCanWarpCursor(MTY_App *ctx)
{
	return true;
}

static Cursor app_create_empty_cursor(Display *display)
{
	Cursor cursor = 0;

	char data[1] = {0};
	Pixmap p = XCreateBitmapFromData(display, XDefaultRootWindow(display), data, 1, 1);

	if (p) {
		XColor c = {0};
		cursor = XCreatePixmapCursor(display, p, p, &c, &c, 0, 0);

		XFreePixmap(display, p);
	}

	return cursor;
}


// App

static void app_hid_connect(struct hdevice *device, void *opaque)
{
	MTY_App *ctx = opaque;

	hid_driver_init(device);

	MTY_Msg msg = {0};
	msg.type = MTY_MSG_CONNECT;
	msg.controller.vid = hid_device_get_vid(device);
	msg.controller.pid = hid_device_get_pid(device);
	msg.controller.id = hid_device_get_id(device);

	ctx->msg_func(&msg, ctx->opaque);
}

static void app_hid_disconnect(struct hdevice *device, void *opaque)
{
	MTY_App *ctx = opaque;

	MTY_Msg msg = {0};
	msg.type = MTY_MSG_DISCONNECT;
	msg.controller.vid = hid_device_get_vid(device);
	msg.controller.pid = hid_device_get_pid(device);
	msg.controller.id = hid_device_get_id(device);

	ctx->msg_func(&msg, ctx->opaque);
}

static void app_hid_report(struct hdevice *device, const void *buf, size_t size, void *opaque)
{
	MTY_App *ctx = opaque;

	MTY_Msg msg = {0};
	hid_driver_state(device, buf, size, &msg);

	if (msg.type != MTY_MSG_NONE && MTY_AppIsActive(ctx))
		ctx->msg_func(&msg, ctx->opaque);
}

static void app_refresh_scale(MTY_App *ctx)
{
	Display *display = XOpenDisplay(NULL);

	const char *dpi = XGetDefault(display, "Xft", "dpi");
	ctx->scale = dpi ? (float) atoi(dpi) / 96.0f : 1.0f;

	XCloseDisplay(display);
}

MTY_App *MTY_AppCreate(MTY_AppFunc appFunc, MTY_MsgFunc msgFunc, void *opaque)
{
	if (!x_dl_global_init())
		return NULL;

	XInitThreads();

	bool r = true;
	MTY_App *ctx = MTY_Alloc(1, sizeof(MTY_App));
	ctx->hotkey = MTY_HashCreate(0);
	ctx->mutex = MTY_MutexCreate();
	ctx->app_func = appFunc;
	ctx->msg_func = msgFunc;
	ctx->opaque = opaque;
	ctx->class_name = MTY_Strdup(MTY_GetFileName(MTY_ProcessName(), false));

	// This may return NULL
	ctx->hid = hid_create(app_hid_connect, app_hid_disconnect, app_hid_report, ctx);

	ctx->display = XOpenDisplay(NULL);
	if (!ctx->display) {
		r = false;
		goto except;
	}

	// Prevents KeyRelease from being generated on keyboard autorepeat (optional)
	if (XkbSetDetectableAutoRepeat)
		XkbSetDetectableAutoRepeat(ctx->display, True, NULL);

	ctx->empty_cursor = app_create_empty_cursor(ctx->display);

	unsigned char mask[3] = {0};
	XISetMask(mask, XI_RawMotion);

	XIEventMask em = {0};
	em.deviceid = XIAllMasterDevices;
	em.mask_len = 3;
	em.mask = mask;

	XISelectEvents(ctx->display, XDefaultRootWindow(ctx->display), &em, 1);

	ctx->im = XOpenIM(ctx->display, NULL, NULL, NULL);
	if (!ctx->im) {
		r = false;
		goto except;
	}

	GLint attr[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
	ctx->vis = glXChooseVisual(ctx->display, 0, attr);
	if (!ctx->vis) {
		r = false;
		goto except;
	}

	ctx->wm_close = XInternAtom(ctx->display, "WM_DELETE_WINDOW", False);
	ctx->wm_ping = XInternAtom(ctx->display, "_NET_WM_PING", False);

	app_refresh_scale(ctx);

	except:

	if (!r)
		MTY_AppDestroy(&ctx);

	return ctx;
}

void MTY_AppDestroy(MTY_App **app)
{
	if (!app || !*app)
		return;

	MTY_App *ctx = *app;

	if (ctx->empty_cursor)
		XFreeCursor(ctx->display, ctx->empty_cursor);

	if (ctx->vis)
		XFree(ctx->vis);

	if (ctx->im)
		XCloseIM(ctx->im);

	if (ctx->display)
		XCloseDisplay(ctx->display);

	hid_destroy(&ctx->hid);

	MTY_HashDestroy(&ctx->hotkey, NULL);
	MTY_MutexDestroy(&ctx->mutex);
	MTY_Free(ctx->clip);
	MTY_Free(ctx->class_name);

	MTY_Free(*app);
	*app = NULL;
}


// Event handling

static void window_text_event(MTY_App *ctx, XEvent *event)
{
	struct window *win = app_get_window(ctx, 0);
	if (!win)
		return;

	MTY_Msg msg = {0};
	msg.type = MTY_MSG_TEXT;

	Status status = 0;
	KeySym ks = 0;

	if (Xutf8LookupString(win->ic, (XKeyPressedEvent *) event, msg.text, 8, &ks, &status) > 0)
		ctx->msg_func(&msg, ctx->opaque);
}

static void app_kb_to_hotkey(MTY_App *app, MTY_Msg *msg)
{
	MTY_Mod mod = msg->keyboard.mod & 0xFF;
	uint32_t hotkey = (uint32_t) (uintptr_t) MTY_HashGetInt(app->hotkey, (mod << 16) | msg->keyboard.key);

	if (hotkey != 0) {
		if (msg->keyboard.pressed) {
			msg->type = MTY_MSG_HOTKEY;
			msg->hotkey = hotkey;

		} else {
			msg->type = MTY_MSG_NONE;
		}
	}
}

static void app_make_movement(MTY_App *app, MTY_Window window)
{
	struct window *win = app_get_window(app, window);
	if (!win)
		return;

	Window root, child;
	unsigned int mask;
	int root_x, root_y, win_x = 0, win_y = 0;
	XQueryPointer(app->display, win->window, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask);

	MTY_Msg msg = {0};
	msg.type = MTY_MSG_MOUSE_MOTION;
	msg.window = window;
	msg.mouseMotion.relative = false;
	msg.mouseMotion.click = true;
	msg.mouseMotion.x = win_x;
	msg.mouseMotion.y = win_y;

	app->msg_func(&msg, app->opaque);
}

static void app_event(MTY_App *ctx, XEvent *event)
{
	MTY_Msg msg = {0};

	switch (event->type) {
		case KeyPress:
			window_text_event(ctx, event);
			// Fall through

		case KeyRelease: {
			msg.keyboard.key = window_keycode_to_key(event->xkey.keycode);

			if (msg.keyboard.key != MTY_KEY_NONE) {
				msg.type = MTY_MSG_KEYBOARD;
				msg.window = app_find_window(ctx, event->xkey.window);
				msg.keyboard.pressed = event->type == KeyPress;
				msg.keyboard.mod = window_keystate_to_keymod(msg.keyboard.key,
					msg.keyboard.pressed, event->xkey.state);
			}
			break;
		}
		case ButtonPress:
		case ButtonRelease:
			msg.type = MTY_MSG_MOUSE_BUTTON;
			msg.window = app_find_window(ctx, event->xbutton.window);
			msg.mouseButton.pressed = event->type == ButtonPress;

			switch (event->xbutton.button) {
				case 1: msg.mouseButton.button = MTY_MOUSE_BUTTON_LEFT;   break;
				case 2: msg.mouseButton.button = MTY_MOUSE_BUTTON_MIDDLE; break;
				case 3: msg.mouseButton.button = MTY_MOUSE_BUTTON_RIGHT;  break;
				case 8: msg.mouseButton.button = MTY_MOUSE_BUTTON_X1;     break;
				case 9: msg.mouseButton.button = MTY_MOUSE_BUTTON_X2;     break;

				// Mouse wheel
				case 4:
				case 5:
				case 6:
				case 7:
					if (event->type == ButtonPress) {
						msg.type = MTY_MSG_MOUSE_WHEEL;
						msg.mouseWheel.x = event->xbutton.button == 6 ? -120 : event->xbutton.button == 7 ?  120 : 0;
						msg.mouseWheel.y = event->xbutton.button == 4 ?  120 : event->xbutton.button == 5 ? -120 : 0;

					} else {
						msg.type = MTY_MSG_NONE;
					}
					break;
			}
			break;
		case MotionNotify:
			if (ctx->relative)
				break;

			msg.type = MTY_MSG_MOUSE_MOTION;
			msg.window = app_find_window(ctx, event->xmotion.window);
			msg.mouseMotion.relative = false;
			msg.mouseMotion.x = event->xmotion.x;
			msg.mouseMotion.y = event->xmotion.y;
			break;
		case ClientMessage: {
			Atom type = (Atom) event->xclient.data.l[0];

			if (type == ctx->wm_close) {
				msg.window = app_find_window(ctx, event->xclient.window);
				msg.type = MTY_MSG_CLOSE;

			// Ping -> Pong
			} else if (type == ctx->wm_ping) {
				Window root = XDefaultRootWindow(ctx->display);

				event->xclient.window = root;
				XSendEvent(ctx->display, root, False, SubstructureRedirectMask | SubstructureNotifyMask, event);
			}
			break;
		}
		case GenericEvent:
			if (!ctx->relative)
				break;

			if (XGetEventData(ctx->display, &event->xcookie)) {
				if (event->xcookie.evtype == XI_RawMotion) {
					const XIRawEvent *re = (const XIRawEvent *) event->xcookie.data;
					const double *input = (const double *) re->raw_values;
					int32_t output[2] = {0};

					for (int32_t i = 0; i < 2; i++)
						if (XIMaskIsSet(re->valuators.mask, i))
							output[i] = (int32_t) lrint(*input++);

					struct window *win = app_get_active_window(ctx);
					if (win && (output[0] != 0 || output[1] != 0)) {
						msg.type = MTY_MSG_MOUSE_MOTION;
						msg.window = win->index;
						msg.mouseMotion.relative = true;
						msg.mouseMotion.x = output[0];
						msg.mouseMotion.y = output[1];
					}
				}
			}
			break;
		case SelectionRequest:
			app_handle_selection_request(ctx, event);
			break;
		case SelectionNotify:
			app_handle_selection_notify(ctx, &event->xselection);
			break;
		case Expose:
			app_refresh_scale(ctx);
			ctx->state++;
			break;
		case FocusIn:
		case FocusOut:
			// Do not respond to NotifyGrab or NotifyUngrab
			if (event->xfocus.mode == NotifyNormal || event->xfocus.mode == NotifyWhileGrabbed) {
				msg.type = MTY_MSG_FOCUS;
				msg.focus = event->type == FocusIn;
			}
			ctx->state++;
			break;
	}

	// Transform keyboard into hotkey
	if (msg.type == MTY_MSG_KEYBOARD)
		app_kb_to_hotkey(ctx, &msg);

	// For robustness, generate a MOUSE_MOTION event to where the click happens
	if (msg.type == MTY_MSG_MOUSE_BUTTON && msg.mouseButton.pressed && !ctx->relative)
		app_make_movement(ctx, msg.window);

	// Handle the message
	if (msg.type != MTY_MSG_NONE)
		ctx->msg_func(&msg, ctx->opaque);
}

static void app_suspend_ss(MTY_App *ctx)
{
	int64_t now = MTY_Timestamp();

	// Keep screen saver disabled in 30s intervals
	if (MTY_TimeDiff(ctx->suspend_ts, now) > 30000.0f) {
		XResetScreenSaver(ctx->display);
		ctx->suspend_ts = now;
	}
}

void MTY_AppRun(MTY_App *ctx)
{
	for (bool cont = true; cont;) {
		// Grab / mouse state evaluation
		if (ctx->state != ctx->prev_state) {
			struct window *win = app_get_active_window(ctx);

			app_apply_keyboard_grab(ctx, win);
			app_apply_mouse_grab(ctx, win);
			app_apply_cursor(ctx, win ? true : false);
			XSync(ctx->display, False);

			ctx->prev_state = ctx->state;
		}

		// Poll selection ownership changes
		app_poll_clipboard(ctx);

		// X11 events
		for (XEvent event; XEventsQueued(ctx->display, QueuedAfterFlush) > 0;) {
			XNextEvent(ctx->display, &event);
			app_event(ctx, &event);
		}

		// evdev events
		if (ctx->hid)
			hid_poll(ctx->hid);

		// Fire app func after all events have been processed
		cont = ctx->app_func(ctx->opaque);

		// Keep screensaver from turning on
		if (ctx->suspend_ss)
			app_suspend_ss(ctx);

		if (ctx->timeout > 0)
			MTY_Sleep(ctx->timeout);
	}
}

void MTY_AppSetTimeout(MTY_App *ctx, uint32_t timeout)
{
	ctx->timeout = timeout;
}

void MTY_AppDetach(MTY_App *app, MTY_Detach type)
{
	if (app->detach != type) {
		app->detach = type;
		app->state++;
	}
}

MTY_Detach MTY_AppGetDetached(MTY_App *app)
{
	return app->detach;
}

void MTY_AppEnableScreenSaver(MTY_App *app, bool enable)
{
	app->suspend_ss = !enable;
}

void MTY_AppGrabKeyboard(MTY_App *app, bool grab)
{
	if (app->kbgrab != grab) {
		app->kbgrab = grab;
		app->state++;
	}
}

void MTY_AppGrabMouse(MTY_App *app, bool grab)
{
	if (app->mgrab != grab) {
		app->mgrab = grab;
		app->state++;
	}
}

void MTY_AppSetRelativeMouse(MTY_App *app, bool relative)
{
	if (app->relative != relative) {
		// FIXME This should keep track of the position where the cursor went into relative,
		// but for now since we usually call WarpCursor whenever we exit relative it
		// doesn't matter

		app->relative = relative;

		struct window *win = app_get_active_window(app);

		app_apply_mouse_grab(app, win);
		app_apply_cursor(app, win ? true : false);
		XSync(app->display, False);
	}
}

bool MTY_AppGetRelativeMouse(MTY_App *app)
{
	return app->relative;
}

bool MTY_AppIsActive(MTY_App *ctx)
{
	return app_get_active_window(ctx) ? true : false;
}

void MTY_AppActivate(MTY_App *app, bool active)
{
	MTY_WindowActivate(app, 0, active);
}

void MTY_AppControllerRumble(MTY_App *app, uint32_t id, uint16_t low, uint16_t high)
{
	if (app->hid)
		hid_driver_rumble(app->hid, id, low, high);
}


// Window

static void window_set_up_wm(MTY_App *app, Window w, const MTY_WindowDesc *desc)
{
	// This function sets up misc things associated with the standardized _NET_WM
	// system for communicating with the window manager

	XSizeHints *shints = XAllocSizeHints();
	shints->flags = PMinSize;
	shints->min_width = desc->minWidth;
	shints->min_height = desc->minHeight;

	XWMHints *wmhints = XAllocWMHints();
	wmhints->input = True;
	wmhints->window_group = (uintptr_t) app;
	wmhints->flags = InputHint | WindowGroupHint;

	XClassHint *chints = XAllocClassHint();
	chints->res_name = app->class_name;
	chints->res_class = app->class_name;

	XSetWMProperties(app->display, w, NULL, NULL, NULL, 0, shints, wmhints, chints);

	XFree(shints);
	XFree(wmhints);
	XFree(chints);

	Atom wintype = XInternAtom(app->display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
	XChangeProperty(app->display, w, XInternAtom(app->display, "_NET_WM_WINDOW_TYPE", False), XA_ATOM, 32,
		PropModeReplace, (unsigned char *) &wintype, 1);

	pid_t pid = getpid();
	XChangeProperty(app->display, w, XInternAtom(app->display, "_NET_WM_PID", False),
		XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &pid, 1);

	Atom protos[2] = {app->wm_close, app->wm_ping};
	XSetWMProtocols(app->display, w, protos, 2);
}

MTY_Window MTY_WindowCreate(MTY_App *app, const char *title, const MTY_WindowDesc *desc)
{
	bool r = true;
	struct window *ctx = MTY_Alloc(1, sizeof(struct window));

	MTY_Window window = app_find_open_window(app);
	if (window == -1) {
		r = false;
		MTY_Log("Maximum windows (MTY_WINDOW_MAX) of %u reached", MTY_WINDOW_MAX);
		goto except;
	}

	ctx->index = window;
	app->windows[window] = ctx;

	Window root = XDefaultRootWindow(app->display);

	XSetWindowAttributes swa = {0};
	swa.colormap = XCreateColormap(app->display, root, app->vis->visual, AllocNone);
	swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask |
		ButtonReleaseMask | PointerMotionMask | FocusChangeMask;

	XWindowAttributes attr = {0};
	XGetWindowAttributes(app->display, root, &attr);

	int32_t desktop_width = XWidthOfScreen(attr.screen);
	int32_t desktop_height = XHeightOfScreen(attr.screen);
	int32_t width = desktop_width;
	int32_t height = desktop_height;
	int32_t x = lrint((float) desc->x * app->scale);
	int32_t y = lrint((float) desc->y * app->scale);

	mty_wsize_client(desc, app->scale, desktop_height, &x, &y, &width, &height);

	if (desc->position == MTY_POSITION_CENTER)
		mty_wsize_center(0, 0, desktop_width, desktop_height, &x, &y, &width, &height);

	ctx->window = XCreateWindow(app->display, root, 0, 0, width, height, 0, app->vis->depth,
		InputOutput, app->vis->visual, CWColormap | CWEventMask, &swa);

	ctx->ic = XCreateIC(app->im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
		XNClientWindow, ctx->window, NULL);

	XMapRaised(app->display, ctx->window);
	XMoveWindow(app->display, ctx->window, x, y);

	MTY_WindowSetTitle(app, window, title);

	window_set_up_wm(app, ctx->window, desc);

	ctx->info.display = app->display;
	ctx->info.vis = app->vis;
	ctx->info.window = ctx->window;

	if (desc->api != MTY_GFX_NONE) {
		if (!MTY_WindowSetGFX(app, window, desc->api, desc->vsync)) {
			r = false;
			goto except;
		}
	}

	except:

	if (!r) {
		MTY_WindowDestroy(app, window);
		window = -1;
	}

	return window;
}

void MTY_WindowDestroy(MTY_App *app, MTY_Window window)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	if (ctx->ic)
		XDestroyIC(ctx->ic);

	XDestroyWindow(app->display, ctx->window);

	MTY_Free(ctx);
	app->windows[window] = NULL;
}

void MTY_WindowSetTitle(MTY_App *app, MTY_Window window, const char *title)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	XStoreName(app->display, ctx->window, title);
}

bool MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	XWindowAttributes attr = {0};
	XGetWindowAttributes(app->display, ctx->window, &attr);

	*width = attr.width;
	*height = attr.height;

	return true;
}

bool MTY_WindowGetScreenSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	XWindowAttributes attr = {0};
	XGetWindowAttributes(app->display, ctx->window, &attr);

	*width = XWidthOfScreen(attr.screen);
	*height = XHeightOfScreen(attr.screen);

	return true;
}

float MTY_WindowGetScale(MTY_App *app, MTY_Window window)
{
	return app->scale;
}

void MTY_WindowEnableFullscreen(MTY_App *app, MTY_Window window, bool fullscreen)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	if (fullscreen != MTY_WindowIsFullscreen(app, window)) {
		XWindowAttributes attr = {0};
		XGetWindowAttributes(app->display, ctx->window, &attr);

		XEvent evt = {0};
		evt.type = ClientMessage;
		evt.xclient.message_type = XInternAtom(app->display, "_NET_WM_STATE", False);
		evt.xclient.format = 32;
		evt.xclient.window = ctx->window;
		evt.xclient.data.l[0] = _NET_WM_STATE_TOGGLE;
		evt.xclient.data.l[1] = XInternAtom(app->display, "_NET_WM_STATE_FULLSCREEN", False);

		XSendEvent(app->display, XRootWindowOfScreen(attr.screen), 0,
			SubstructureNotifyMask | SubstructureRedirectMask, &evt);

		XSync(app->display, False);
	}
}

bool MTY_WindowIsFullscreen(MTY_App *app, MTY_Window window)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	Atom type;
	int format;
	unsigned char *value = NULL;
	unsigned long bytes, n;

	if (XGetWindowProperty(app->display, ctx->window, XInternAtom(app->display, "_NET_WM_STATE", False),
		0, 1024, False, XA_ATOM, &type, &format, &n, &bytes, &value) == Success)
	{
		Atom *atoms = (Atom *) value;

		for (unsigned long x = 0; x < n; x++)
			if (atoms[x] == XInternAtom(app->display, "_NET_WM_STATE_FULLSCREEN", False))
				return true;
	}

	return false;
}

void MTY_WindowActivate(MTY_App *app, MTY_Window window, bool active)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	if (active) {
		XMapRaised(app->display, ctx->window);

		XWindowAttributes attr = {0};
		XGetWindowAttributes(app->display, ctx->window, &attr);

		XEvent evt = {0};
		evt.type = ClientMessage;
		evt.xclient.message_type = XInternAtom(app->display, "_NET_ACTIVE_WINDOW", False);
		evt.xclient.format = 32;
		evt.xclient.window = ctx->window;
		evt.xclient.data.l[0] = 1;
		evt.xclient.data.l[1] = CurrentTime;

		XSendEvent(app->display, XRootWindowOfScreen(attr.screen), 0,
			SubstructureNotifyMask | SubstructureRedirectMask, &evt);

		XSetInputFocus(app->display, ctx->window, RevertToNone, CurrentTime);

		XSync(app->display, False);

	} else {
		XWithdrawWindow(app->display, ctx->window);
	}

	XSync(app->display, False);
}

void MTY_WindowWarpCursor(MTY_App *app, MTY_Window window, uint32_t x, uint32_t y)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	MTY_AppSetRelativeMouse(app, false);

	XWarpPointer(app->display, None, ctx->window, 0, 0, 0, 0, x, y);
	XSync(app->display, False);
}

bool MTY_WindowIsVisible(MTY_App *app, MTY_Window window)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	XWindowAttributes attr = {0};
	XGetWindowAttributes(app->display, ctx->window, &attr);

	return attr.map_state == IsViewable;
}

bool MTY_WindowIsActive(MTY_App *app, MTY_Window window)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return false;

	Window w = 0;
	int32_t revert = 0;
	XGetInputFocus(app->display, &w, &revert);

	return ctx->window == w;
}

bool MTY_WindowExists(MTY_App *app, MTY_Window window)
{
	return app_get_window(app, window) ? true : false;
}


// Window Private

void mty_mty_window_set_gfx(MTY_App *app, MTY_Window window, MTY_GFX api, struct gfx_ctx *gfx_ctx)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return;

	ctx->api = api;
	ctx->gfx_ctx = gfx_ctx;
}

MTY_GFX mty_mty_window_get_gfx(MTY_App *app, MTY_Window window, struct gfx_ctx **gfx_ctx)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return MTY_GFX_NONE;

	if (gfx_ctx)
		*gfx_ctx = ctx->gfx_ctx;

	return ctx->api;
}

void *mty_mty_window_get_native(MTY_App *app, MTY_Window window)
{
	struct window *ctx = app_get_window(app, window);
	if (!ctx)
		return NULL;

	return (void *) &ctx->info;
}


// Misc

void *MTY_GLGetProcAddress(const char *name)
{
	if (!x_dl_global_init())
		return NULL;

	return glXGetProcAddress((const GLubyte *) name);
}


// Unimplemented

void MTY_AppSetTray(MTY_App *app, const char *tooltip, const MTY_MenuItem *items, uint32_t len)
{
}

void MTY_AppRemoveTray(MTY_App *app)
{
}

void MTY_AppNotification(MTY_App *app, const char *title, const char *msg)
{
}

void MTY_AppShowSoftKeyboard(MTY_App *app, bool show)
{
}

bool MTY_AppSoftKeyboardIsShowing(MTY_App *app)
{
	return false;
}

void MTY_AppSetOrientation(MTY_App *app, MTY_Orientation orientation)
{
}

void MTY_AppEnableGlobalHotkeys(MTY_App *app, bool enable)
{
}

void MTY_AppEnablePen(MTY_App *ctx, bool enable)
{
}

MTY_GFXState MTY_WindowGFXState(MTY_App *app, MTY_Window window)
{
	return MTY_GFX_STATE_NORMAL;
}

void MTY_AppSetInputMode(MTY_App *ctx, MTY_Input mode)
{
}