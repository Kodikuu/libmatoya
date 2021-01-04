// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "GL/glcorearb30.h"


// X interface

// Reference: https://code.woboq.org/qt5/include/X11/

#define _Xconst                   const

#define Success                   0

#define None                      0L
#define NoSymbol                  0L
#define Bool                      int
#define True                      1
#define False                     0
#define Status                    int
#define AllocNone                 0

#define RevertToNone              ((int) None)

#define InputOutput               1
#define InputOnly                 2

#define NoEventMask               0L
#define KeyPressMask              (1L<<0)
#define KeyReleaseMask            (1L<<1)
#define ButtonPressMask           (1L<<2)
#define ButtonReleaseMask         (1L<<3)
#define EnterWindowMask           (1L<<4)
#define LeaveWindowMask           (1L<<5)
#define PointerMotionMask         (1L<<6)
#define PointerMotionHintMask     (1L<<7)
#define Button1MotionMask         (1L<<8)
#define Button2MotionMask         (1L<<9)
#define Button3MotionMask         (1L<<10)
#define Button4MotionMask         (1L<<11)
#define Button5MotionMask         (1L<<12)
#define ButtonMotionMask          (1L<<13)
#define KeymapStateMask           (1L<<14)
#define ExposureMask              (1L<<15)
#define VisibilityChangeMask      (1L<<16)
#define StructureNotifyMask       (1L<<17)
#define ResizeRedirectMask        (1L<<18)
#define SubstructureNotifyMask    (1L<<19)
#define SubstructureRedirectMask  (1L<<20)
#define FocusChangeMask           (1L<<21)
#define PropertyChangeMask        (1L<<22)
#define ColormapChangeMask        (1L<<23)
#define OwnerGrabButtonMask       (1L<<24)

#define PropModeReplace           0
#define PropModePrepend           1
#define PropModeAppend            2

#define ShiftMask                 (1<<0)
#define LockMask                  (1<<1)
#define ControlMask               (1<<2)
#define Mod1Mask                  (1<<3)
#define Mod2Mask                  (1<<4)
#define Mod3Mask                  (1<<5)
#define Mod4Mask                  (1<<6)
#define Mod5Mask                  (1<<7)

#define KeyPress                  2
#define KeyRelease                3
#define ButtonPress               4
#define ButtonRelease             5
#define MotionNotify              6
#define EnterNotify               7
#define LeaveNotify               8
#define FocusIn                   9
#define FocusOut                  10
#define KeymapNotify              11
#define Expose                    12
#define GraphicsExpose            13
#define NoExpose                  14
#define VisibilityNotify          15
#define CreateNotify              16
#define DestroyNotify             17
#define UnmapNotify               18
#define MapNotify                 19
#define MapRequest                20
#define ReparentNotify            21
#define ConfigureNotify           22
#define ConfigureRequest          23
#define GravityNotify             24
#define ResizeRequest             25
#define CirculateNotify           26
#define CirculateRequest          27
#define PropertyNotify            28
#define SelectionClear            29
#define SelectionRequest          30
#define SelectionNotify           31
#define ColormapNotify            32
#define ClientMessage             33
#define MappingNotify             34
#define GenericEvent              35
#define LASTEvent                 36

#define CWBackPixmap              (1L<<0)
#define CWBackPixel               (1L<<1)
#define CWBorderPixmap            (1L<<2)
#define CWBorderPixel             (1L<<3)
#define CWBitGravity              (1L<<4)
#define CWWinGravity              (1L<<5)
#define CWBackingStore            (1L<<6)
#define CWBackingPlanes           (1L<<7)
#define CWBackingPixel            (1L<<8)
#define CWOverrideRedirect        (1L<<9)
#define CWSaveUnder               (1L<<10)
#define CWEventMask               (1L<<11)
#define CWDontPropagate           (1L<<12)
#define CWColormap                (1L<<13)
#define CWCursor                  (1L<<14)

#define IsUnmapped                0
#define IsUnviewable              1
#define IsViewable                2

#define QueuedAlready             0
#define QueuedAfterReading        1
#define QueuedAfterFlush          2

#define XNInputStyle              "inputStyle"
#define XNClientWindow            "clientWindow"

#define XIMPreeditNothing         0x0008L
#define XIMStatusNothing          0x0400L

#define CurrentTime               0L

#define GrabModeSync              0
#define GrabModeAsync             1

#define NotifyNormal              0
#define NotifyGrab                1
#define NotifyUngrab              2
#define NotifyWhileGrabbed        3

#define XA_PRIMARY                ((Atom) 1)
#define XA_ATOM                   ((Atom) 4)
#define XA_CARDINAL               ((Atom) 6)
#define XA_CUT_BUFFER0            ((Atom) 9)
#define XA_STRING                 ((Atom) 31)

#define USPosition                (1L << 0)
#define USSize                    (1L << 1)
#define PPosition                 (1L << 2)
#define PSize                     (1L << 3)
#define PMinSize                  (1L << 4)
#define PMaxSize                  (1L << 5)
#define PResizeInc                (1L << 6)
#define PAspect                   (1L << 7)
#define PBaseSize                 (1L << 8)
#define PWinGravity               (1L << 9)

#define InputHint                 (1L << 0)
#define WindowGroupHint           (1L << 6)

#define _NET_WM_STATE_REMOVE      0
#define _NET_WM_STATE_ADD         1
#define _NET_WM_STATE_TOGGLE      2

typedef unsigned long VisualID;
typedef unsigned long XID;
typedef unsigned long Time;
typedef unsigned long Atom;
typedef char *XPointer;
typedef XID Window;
typedef XID Drawable;
typedef XID Colormap;
typedef XID Pixmap;
typedef XID Cursor;
typedef XID KeySym;
typedef struct _XDisplay Display;
typedef struct _XVisual Visual;
typedef struct _XScreen Screen;
typedef unsigned char KeyCode;

typedef struct {
	unsigned long pixel;
	unsigned short red, green, blue;
	char flags;
	char pad;
} XColor;

typedef struct {
	Visual *visual;
	VisualID visualid;
	int screen;
	unsigned int depth;
	int class;
	unsigned long red_mask;
	unsigned long green_mask;
	unsigned long blue_mask;
	int colormap_size;
	int bits_per_rgb;
} XVisualInfo;

typedef struct {
	Pixmap background_pixmap;
	unsigned long background_pixel;
	Pixmap border_pixmap;
	unsigned long border_pixel;
	int bit_gravity;
	int win_gravity;
	int backing_store;
	unsigned long backing_planes;
	unsigned long backing_pixel;
	Bool save_under;
	long event_mask;
	long do_not_propagate_mask;
	Bool override_redirect;
	Colormap colormap;
	Cursor cursor;
} XSetWindowAttributes;

typedef struct {
	int x;
	int y;
	int width;
	int height;
	int border_width;
	int depth;
	Visual *visual;
	Window root;
	int class;
	int bit_gravity;
	int win_gravity;
	int backing_store;
	unsigned long backing_planes;
	unsigned long backing_pixel;
	Bool save_under;
	Colormap colormap;
	Bool map_installed;
	int map_state;
	long all_event_masks;
	long your_event_mask;
	long do_not_propagate_mask;
	Bool override_redirect;
	Screen *screen;
} XWindowAttributes;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	int extension;
	int evtype;
	unsigned int cookie;
	void *data;
} XGenericEventCookie;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	Window root;
	Window subwindow;
	Time time;
	int x;
	int y;
	int x_root;
	int y_root;
	unsigned int state;
	unsigned int keycode;
	Bool same_screen;
} XKeyEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	Atom message_type;
	int format;
	union {
		char b[20];
		short s[10];
		long l[5];
	} data;
} XClientMessageEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window owner;
	Window requestor;
	Atom selection;
	Atom target;
	Atom property;
	Time time;
} XSelectionRequestEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window requestor;
	Atom selection;
	Atom target;
	Atom property;
	Time time;
} XSelectionEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	Window root;
	Window subwindow;
	Time time;
	int x;
	int y;
	int x_root;
	int y_root;
	unsigned int state;
	unsigned int button;
	Bool same_screen;
} XButtonEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	Window root;
	Window subwindow;
	Time time;
	int x;
	int y;
	int x_root;
	int y_root;
	unsigned int state;
	char is_hint;
	Bool same_screen;
} XMotionEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	int mode;
	int detail;
} XFocusChangeEvent;

typedef union _XEvent {
	int type;
	XKeyEvent xkey;
	XButtonEvent xbutton;
	XMotionEvent xmotion;
	XFocusChangeEvent xfocus;
	XSelectionRequestEvent xselectionrequest;
	XSelectionEvent xselection;
	XClientMessageEvent xclient;
	XGenericEventCookie xcookie;
	long pad[24];
} XEvent;

typedef struct Hints {
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long inputMode;
	unsigned long status;
} Hints;

typedef struct {
	unsigned char *value;
	Atom encoding;
	int format;
	unsigned long nitems;
} XTextProperty;

typedef struct {
	long flags;
	int x, y;
	int width, height;
	int min_width, min_height;
	int max_width, max_height;
	int width_inc, height_inc;
	struct {
		int x;
		int y;
	} min_aspect, max_aspect;
	int base_width, base_height;
	int win_gravity;
} XSizeHints;

typedef struct {
	long flags;
	Bool input;
	int initial_state;
	Pixmap icon_pixmap;
	Window icon_window;
	int icon_x, icon_y;
	Pixmap icon_mask;
	XID window_group;
} XWMHints;

typedef struct {
	char *res_name;
	char *res_class;
} XClassHint;

typedef XKeyEvent XKeyPressedEvent;
typedef XKeyEvent XKeyReleasedEvent;

struct _XrmHashBucketRec;
typedef struct _XIM *XIM;
typedef struct _XIC *XIC;

static Display *(*XOpenDisplay)(const char *display_name);
static int (*XCloseDisplay)(Display *display);
static Window (*XDefaultRootWindow)(Display *display);
static Window (*XRootWindowOfScreen)(Screen *screen);
static Colormap (*XCreateColormap)(Display *display, Window w, Visual *visual, int alloc);
static Window (*XCreateWindow)(Display *display, Window parent, int x, int y, unsigned int width,
	unsigned int height, unsigned int border_width, int depth, unsigned int class, Visual *visual,
	unsigned long valuemask, XSetWindowAttributes *attributes);
static int (*XWithdrawWindow)(Display *display, Window w);
static int (*XMapRaised)(Display *display, Window w);
static int (*XSetInputFocus)(Display *display, Window focus, int revert_to, Time time);
static int (*XStoreName)(Display *display, Window w, const char *window_name);
static Status (*XGetWindowAttributes)(Display *display, Window w, XWindowAttributes *window_attributes_return);
static KeySym (*XLookupKeysym)(XKeyEvent *key_event, int index);
static Status (*XSetWMProtocols)(Display *display, Window w, Atom *protocols, int count);
static Atom (*XInternAtom)(Display *display, const char *atom_name, Bool only_if_exists);
static int (*XNextEvent)(Display *display, XEvent *event_return);
static int (*XEventsQueued)(Display *display, int mode);
static int (*XMoveWindow)(Display *display, Window w, int x, int y);
static int (*XChangeProperty)(Display *display, Window w, Atom property, Atom type, int format, int mode, const unsigned char *data, int nelements);
static int (*XGetInputFocus)(Display *display, Window *focus_return, int *revert_to_return);
static char *(*XGetDefault)(Display *display, const char *program, const char *option);
static int (*XWidthOfScreen)(Screen *screen);
static int (*XHeightOfScreen)(Screen *screen);
static int (*XDestroyWindow)(Display *display, Window w);
static int (*XFree)(void *data);
static Status (*XInitThreads)(void);
static int (*Xutf8LookupString)(XIC ic, XKeyPressedEvent *event, char *buffer_return, int bytes_buffer,
	KeySym *keysym_return, Status *status_return);
static XIM (*XOpenIM)(Display *dpy, struct _XrmHashBucketRec *rdb, char *res_name, char *res_class);
static Status (*XCloseIM)(XIM im);
static XIC (*XCreateIC)(XIM im, ...);
static void (*XDestroyIC)(XIC ic);
static Bool (*XGetEventData)(Display *dpy, XGenericEventCookie *cookie);
static int (*XGrabPointer)(Display *display, Window grab_window, Bool owner_events, unsigned int event_mask, int pointer_mode,
	int keyboard_mode, Window confine_to, Cursor cursor, Time time);
static int (*XUngrabPointer)(Display *display, Time time);
static int (*XGrabKeyboard)(Display *display, Window grab_window, Bool owner_events, int pointer_mode, int keyboard_mode, Time time);
static int (*XUngrabKeyboard)(Display *display, Time time);
static int (*XWarpPointer)(Display *display, Window src_w, Window dest_w, int src_x, int src_y, unsigned int src_width,
	unsigned int src_height, int dest_x, int dest_y);
static int (*XSync)(Display *display, Bool discard);
static Pixmap (*XCreateBitmapFromData)(Display *display, Drawable d, _Xconst char *data, unsigned int width, unsigned int height);
static Cursor (*XCreatePixmapCursor)(Display *display, Pixmap source, Pixmap mask, XColor *foreground_color,
	XColor *background_color, unsigned int x, unsigned int y);
static int (*XFreePixmap)(Display *display, Pixmap pixmap);
static int (*XDefineCursor)(Display *display, Window w, Cursor cursor);
static int (*XFreeCursor)(Display *display, Cursor cursor);
static Window (*XGetSelectionOwner)(Display *display, Atom selection);
static int (*XSetSelectionOwner)(Display *display, Atom selection, Window owner, Time time);
static char *(*XKeysymToString)(KeySym keysym);
static void (*XConvertCase)(KeySym keysym, KeySym *lower_return, KeySym *upper_return);
static Bool (*XQueryPointer)(Display *display, Window w, Window *root_return, Window *child_return,
	int *root_x_return, int *root_y_return, int *win_x_return, int *win_y_return, unsigned int *mask_return);
static int (*XGetWindowProperty)(Display *display, Window w, Atom property, long long_offset, long long_length,
	Bool delete, Atom req_type, Atom *actual_type_return, int *actual_format_return, unsigned long *nitems_return,
	unsigned long *bytes_after_return, unsigned char **prop_return);
static Status (*XSendEvent)(Display *display, Window w, Bool propagate, long event_mask, XEvent *event_send);
static int (*XConvertSelection)(Display *display, Atom selection, Atom target, Atom property, Window requestor, Time time);
static void (*XSetWMProperties)(Display *display, Window w, XTextProperty *window_name, XTextProperty *icon_name, char **argv,
	int argc, XSizeHints *normal_hints, XWMHints *wm_hints, XClassHint *class_hints);
static XSizeHints *(*XAllocSizeHints)(void);
static XWMHints *(*XAllocWMHints)(void);
static XClassHint *(*XAllocClassHint)(void);
static int (*XResetScreenSaver)(Display *display);


// XI2 interface

// Reference: https://code.woboq.org/kde/include/X11/extensions/

#define XIAllMasterDevices 1

#define XI_RawMotion       17

#define XISetMask(ptr, event) \
	(((unsigned char *) (ptr))[(event) >> 3] |= (1 << ((event) & 7)))

typedef struct {
	int deviceid;
	int mask_len;
	unsigned char *mask;
} XIEventMask;

typedef struct {
	int mask_len;
	unsigned char *mask;
	double *values;
} XIValuatorState;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	int extension;
	int evtype;
	Time time;
	int deviceid;
	int sourceid;
	int detail;
	int flags;
	XIValuatorState valuators;
	double *raw_values;
} XIRawEvent;

static int (*XISelectEvents)(Display *dpy, Window win, XIEventMask *masks, int num_masks);


// Xcursor interface

typedef int XcursorBool;
typedef unsigned int XcursorUInt;
typedef XcursorUInt XcursorDim;
typedef XcursorUInt XcursorPixel;

typedef struct _XcursorImage {
	XcursorUInt version;
	XcursorDim size;
	XcursorDim width;
	XcursorDim height;
	XcursorDim xhot;
	XcursorDim yhot;
	XcursorUInt delay;
	XcursorPixel *pixels;
} XcursorImage;

static XcursorImage *(*XcursorImageCreate)(int width, int height);
static Cursor (*XcursorImageLoadCursor)(Display *dpy, const XcursorImage *image);
static void (*XcursorImageDestroy)(XcursorImage *image);


// GLX interface

// Reference: https://code.woboq.org/qt5/include/GL/

enum _GLX_CONFIGS {
	GLX_USE_GL           = 1,
	GLX_BUFFER_SIZE      = 2,
	GLX_LEVEL            = 3,
	GLX_RGBA             = 4,
	GLX_DOUBLEBUFFER     = 5,
	GLX_STEREO           = 6,
	GLX_AUX_BUFFERS      = 7,
	GLX_RED_SIZE         = 8,
	GLX_GREEN_SIZE       = 9,
	GLX_BLUE_SIZE        = 10,
	GLX_ALPHA_SIZE       = 11,
	GLX_DEPTH_SIZE       = 12,
	GLX_STENCIL_SIZE     = 13,
	GLX_ACCUM_RED_SIZE   = 14,
	GLX_ACCUM_GREEN_SIZE = 15,
	GLX_ACCUM_BLUE_SIZE  = 16,
	GLX_ACCUM_ALPHA_SIZE = 17,
};

typedef XID GLXDrawable;
typedef struct GLXContext * GLXContext;

static void *(*glXGetProcAddress)(const GLubyte *procName);
static void (*glXSwapBuffers)(Display *dpy, GLXDrawable drawable);
static XVisualInfo *(*glXChooseVisual)(Display *dpy, int screen, int *attribList);
static GLXContext (*glXCreateContext)(Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct);
static Bool (*glXMakeCurrent)(Display *dpy, GLXDrawable drawable, GLXContext ctx);
static void (*glXSwapIntervalEXT)(Display *dpy, GLXDrawable drawable, int interval);
static void (*glXDestroyContext)(Display *dpy, GLXContext ctx);
static GLXContext (*glXGetCurrentContext)(void);


// Helper window struct

struct xinfo {
	Display *display;
	XVisualInfo *vis;
	Window window;
};


// Runtime open

static MTY_Atomic32 X_DL_LOCK;
static MTY_SO *X_DL_GLX_SO;
static MTY_SO *X_DL_XI2_SO;
static MTY_SO *X_DL_XCURSOR_SO;
static MTY_SO *X_DL_SO;
static bool X_DL_INIT;

static void x_dl_global_destroy(void)
{
	MTY_GlobalLock(&X_DL_LOCK);

	MTY_SOUnload(&X_DL_GLX_SO);
	MTY_SOUnload(&X_DL_XI2_SO);
	MTY_SOUnload(&X_DL_XCURSOR_SO);
	MTY_SOUnload(&X_DL_SO);
	X_DL_INIT = false;

	MTY_GlobalUnlock(&X_DL_LOCK);
}

static bool x_dl_global_init(void)
{
	MTY_GlobalLock(&X_DL_LOCK);

	if (!X_DL_INIT) {
		bool r = true;

		X_DL_SO = MTY_SOLoad("libX11.so.6");
		X_DL_XI2_SO = MTY_SOLoad("libXi.so.6");
		X_DL_XCURSOR_SO = MTY_SOLoad("libXcursor.so.1");
		X_DL_GLX_SO = MTY_SOLoad("libGL.so.1");

		if (!X_DL_SO || !X_DL_GLX_SO || !X_DL_XI2_SO || !X_DL_XCURSOR_SO) {
			r = false;
			goto except;
		}

		#define X_DL_LOAD_SYM(so, name) \
			name = MTY_SOGetSymbol(so, #name); \
			if (!name) {r = false; goto except;}

		X_DL_LOAD_SYM(X_DL_SO, XOpenDisplay);
		X_DL_LOAD_SYM(X_DL_SO, XCloseDisplay);
		X_DL_LOAD_SYM(X_DL_SO, XDefaultRootWindow);
		X_DL_LOAD_SYM(X_DL_SO, XRootWindowOfScreen);
		X_DL_LOAD_SYM(X_DL_SO, XCreateColormap);
		X_DL_LOAD_SYM(X_DL_SO, XCreateWindow);
		X_DL_LOAD_SYM(X_DL_SO, XWithdrawWindow);
		X_DL_LOAD_SYM(X_DL_SO, XMapRaised);
		X_DL_LOAD_SYM(X_DL_SO, XSetInputFocus);
		X_DL_LOAD_SYM(X_DL_SO, XStoreName);
		X_DL_LOAD_SYM(X_DL_SO, XGetWindowAttributes);
		X_DL_LOAD_SYM(X_DL_SO, XLookupKeysym);
		X_DL_LOAD_SYM(X_DL_SO, XSetWMProtocols);
		X_DL_LOAD_SYM(X_DL_SO, XInternAtom);
		X_DL_LOAD_SYM(X_DL_SO, XNextEvent);
		X_DL_LOAD_SYM(X_DL_SO, XEventsQueued);
		X_DL_LOAD_SYM(X_DL_SO, XMoveWindow);
		X_DL_LOAD_SYM(X_DL_SO, XChangeProperty);
		X_DL_LOAD_SYM(X_DL_SO, XGetInputFocus);
		X_DL_LOAD_SYM(X_DL_SO, XGetDefault);
		X_DL_LOAD_SYM(X_DL_SO, XWidthOfScreen);
		X_DL_LOAD_SYM(X_DL_SO, XHeightOfScreen);
		X_DL_LOAD_SYM(X_DL_SO, XDestroyWindow);
		X_DL_LOAD_SYM(X_DL_SO, XFree);
		X_DL_LOAD_SYM(X_DL_SO, XInitThreads);
		X_DL_LOAD_SYM(X_DL_SO, Xutf8LookupString);
		X_DL_LOAD_SYM(X_DL_SO, XOpenIM);
		X_DL_LOAD_SYM(X_DL_SO, XCloseIM);
		X_DL_LOAD_SYM(X_DL_SO, XCreateIC);
		X_DL_LOAD_SYM(X_DL_SO, XDestroyIC);
		X_DL_LOAD_SYM(X_DL_SO, XGetEventData);
		X_DL_LOAD_SYM(X_DL_SO, XGrabPointer);
		X_DL_LOAD_SYM(X_DL_SO, XUngrabPointer);
		X_DL_LOAD_SYM(X_DL_SO, XGrabKeyboard);
		X_DL_LOAD_SYM(X_DL_SO, XUngrabKeyboard);
		X_DL_LOAD_SYM(X_DL_SO, XWarpPointer);
		X_DL_LOAD_SYM(X_DL_SO, XSync);
		X_DL_LOAD_SYM(X_DL_SO, XCreateBitmapFromData);
		X_DL_LOAD_SYM(X_DL_SO, XCreatePixmapCursor);
		X_DL_LOAD_SYM(X_DL_SO, XFreePixmap);
		X_DL_LOAD_SYM(X_DL_SO, XDefineCursor);
		X_DL_LOAD_SYM(X_DL_SO, XFreeCursor);
		X_DL_LOAD_SYM(X_DL_SO, XGetSelectionOwner);
		X_DL_LOAD_SYM(X_DL_SO, XSetSelectionOwner);
		X_DL_LOAD_SYM(X_DL_SO, XKeysymToString);
		X_DL_LOAD_SYM(X_DL_SO, XConvertCase);
		X_DL_LOAD_SYM(X_DL_SO, XQueryPointer);
		X_DL_LOAD_SYM(X_DL_SO, XGetWindowProperty);
		X_DL_LOAD_SYM(X_DL_SO, XSendEvent);
		X_DL_LOAD_SYM(X_DL_SO, XConvertSelection);
		X_DL_LOAD_SYM(X_DL_SO, XSetWMProperties);
		X_DL_LOAD_SYM(X_DL_SO, XAllocSizeHints);
		X_DL_LOAD_SYM(X_DL_SO, XAllocWMHints);
		X_DL_LOAD_SYM(X_DL_SO, XAllocClassHint);
		X_DL_LOAD_SYM(X_DL_SO, XResetScreenSaver);

		X_DL_LOAD_SYM(X_DL_XI2_SO, XISelectEvents);

		X_DL_LOAD_SYM(X_DL_XCURSOR_SO, XcursorImageCreate);
		X_DL_LOAD_SYM(X_DL_XCURSOR_SO, XcursorImageLoadCursor);
		X_DL_LOAD_SYM(X_DL_XCURSOR_SO, XcursorImageDestroy);

		X_DL_LOAD_SYM(X_DL_GLX_SO, glXGetProcAddress);
		X_DL_LOAD_SYM(X_DL_GLX_SO, glXSwapBuffers);
		X_DL_LOAD_SYM(X_DL_GLX_SO, glXChooseVisual);
		X_DL_LOAD_SYM(X_DL_GLX_SO, glXCreateContext);
		X_DL_LOAD_SYM(X_DL_GLX_SO, glXMakeCurrent);
		X_DL_LOAD_SYM(X_DL_GLX_SO, glXSwapIntervalEXT);
		X_DL_LOAD_SYM(X_DL_GLX_SO, glXDestroyContext);
		X_DL_LOAD_SYM(X_DL_GLX_SO, glXGetCurrentContext);

		except:

		if (!r)
			x_dl_global_destroy();

		X_DL_INIT = r;
	}

	MTY_GlobalUnlock(&X_DL_LOCK);

	return X_DL_INIT;
}
