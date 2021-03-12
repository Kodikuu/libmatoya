// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "hid/hid.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <linux/input.h>

#include "udev-dl.h"

// https://github.com/torvalds/linux/blob/master/include/uapi/linux/input.h
// https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h

#define HID_FD_MAX 33

struct hid {
	bool init_scan;
	struct udev *udev;
	struct udev_monitor *udev_monitor;
	MTY_Hash *devices;
	MTY_Hash *devices_rev;
	HID_CONNECT connect;
	HID_DISCONNECT disconnect;
	HID_REPORT report;
	struct pollfd fds[HID_FD_MAX];
	void *opaque;
};

struct hat {
	bool up;
	bool right;
	bool down;
	bool left;
};

struct hdevice {
	MTY_ControllerEvent state;
	struct ff_effect ff;
	bool gamepad; // PS4, XInput
	bool rumble; // Can rumble
	struct hat hat;
	uint8_t slot;
	uint32_t id;
	uint16_t vid;
	uint16_t pid;

	struct {
		uint8_t slot;
		int32_t min;
		int32_t max;
	} ainfo[ABS_CNT];
};

static void hid_device_destroy(void *hdevice)
{
	if (!hdevice)
		return;

	struct hdevice *ctx = hdevice;

	MTY_Free(ctx);
}

static uint8_t hid_find_slot(struct hid *ctx)
{
	for (uint8_t x = 1; x < HID_FD_MAX; x++)
		if (ctx->fds[x].fd == -1)
			return x;

	return 0;
}

static void hid_device_add(struct hid *ctx, const char *devnode, const char *syspath)
{
	struct hdevice *hdev = MTY_HashGet(ctx->devices, devnode);
	if (hdev)
		return;

	uint8_t slot = hid_find_slot(ctx);
	if (slot == 0)
		return;

	int32_t fd = open(devnode, O_RDWR | O_NONBLOCK);

	if (fd >= 0) {
		uint64_t evs = 0;
		ioctl(fd, EVIOCGBIT(0, sizeof(uint64_t)), &evs);

		// There is no great way to tell what kind of device this is. Filter by
		// devices that have EV_KEY (keys and buttons) and EV_ABS (absolute axis)
		if ((evs & (1 << EV_KEY)) && (evs & (1 << EV_ABS))) {
			hdev = MTY_Alloc(1, sizeof(struct hdevice));
			hdev->id = fd;
			hdev->slot = slot;

			hdev->state.type = MTY_CTYPE_DEFAULT;
			hdev->state.numValues = 1; // There's always a dummy 'hat' DPAD
			hdev->state.numButtons = 15; // There's no good way to know how many buttons the device has
			hdev->state.id = hdev->id;

			// VID/PID
			struct input_id ids = {0};
			ioctl(fd, EVIOCGID, &ids);
			hdev->state.vid = ids.vendor;
			hdev->state.pid = ids.product;

			// Set up rumble
			hdev->ff.type = FF_RUMBLE;
			hdev->ff.replay.length = 1000;
			hdev->ff.id = -1;

			uint64_t features[2] = {0};
			if (ioctl(fd, EVIOCGBIT(EV_FF, 2 * sizeof(uint64_t)), features) != -1)
				hdev->rumble = features[1] & (1 << (FF_RUMBLE - 64));

			// Discover axis
			for (uint8_t x = 0x00; x < ABS_CNT; x++) {
				if (x != ABS_HAT0X && x != ABS_HAT0Y) {
					struct input_absinfo info = {0};
					ioctl(fd, EVIOCGABS(x), &info);

					if (info.minimum != 0 || info.maximum != 0) {
						hdev->ainfo[x].slot = hdev->state.numValues++;
						hdev->ainfo[x].min = info.minimum;
						hdev->ainfo[x].max = info.maximum;
					}
				}
			}

			ctx->fds[slot].fd = fd;
			MTY_HashSet(ctx->devices, devnode, hdev);
			MTY_HashSetInt(ctx->devices_rev, hdev->id, hdev);

			ctx->connect(hdev, ctx->opaque);

		} else {
			close(fd);
		}

	} else {
		// Many event devnodes are not readable by non-root (like mouse and keyboard)
		// Joysticks usually are
		if (errno != EACCES)
			MTY_Log("'open' failed with errno %d", errno);
	}
}

static void hid_device_remove(struct hid *ctx, const char *devnode)
{
	struct hdevice *hdev = MTY_HashPop(ctx->devices, devnode);
	if (!hdev)
		return;

	ctx->disconnect(hdev, ctx->opaque);
	int32_t *fd = &ctx->fds[hdev->slot].fd;

	if (*fd >= 0) {
		close(*fd);
		*fd = -1;
	}

	MTY_HashPopInt(ctx->devices_rev, hdev->id);
	hid_device_destroy(hdev);
}

static void hid_new_device(struct hid *ctx)
{
	struct udev_device *dev = udev_monitor_receive_device(ctx->udev_monitor);
	if (!dev)
		return;

	const char *action = udev_device_get_action(dev);
	const char *devnode = udev_device_get_devnode(dev);
	const char *syspath = udev_device_get_syspath(dev);
	if (!action || !devnode)
		goto except;

	if (!strstr(devnode, "/event"))
		goto except;

	if (!strcmp(action, "add")) {
		hid_device_add(ctx, devnode, syspath);

	} else if (!strcmp(action, "remove")) {
		hid_device_remove(ctx, devnode);
	}

	except:

	udev_device_unref(dev);
}

static void hid_set_hat(struct hdevice *ctx, uint8_t type, const struct input_event *event)
{
	struct hat *h = &ctx->hat;

	if (type == ABS_HAT0X || type == ABS_HAT0Y) {
		if (type == ABS_HAT0X) {
			h->right = event->value > 0;
			h->left = event->value < 0;

		} else if (type == ABS_HAT0Y) {
			h->up = event->value < 0;
			h->down = event->value > 0;
		}
	}

	ctx->state.values[0].data = h->up && h->right ? 1 : h->right && h->down ? 3 : h->down && h->left ? 5 :
		h->left && h->up ? 7 : h->up ? 0 : h->right ? 2 : h->down ? 4 : h->left ? 6 : 8;
	ctx->state.values[0].usage = 0x39;
	ctx->state.values[0].min = 0;
	ctx->state.values[0].max = 7;
}

static MTY_CButton hid_button(uint16_t type)
{
	switch (type) {
		// Gamepad
		case BTN_A: return MTY_CBUTTON_A;
		case BTN_B: return MTY_CBUTTON_B;
		case BTN_X: return MTY_CBUTTON_X;
		case BTN_Y: return MTY_CBUTTON_Y;
		case BTN_TL: return MTY_CBUTTON_LEFT_SHOULDER;
		case BTN_TR: return MTY_CBUTTON_RIGHT_SHOULDER;
		case BTN_TL2: return MTY_CBUTTON_LEFT_TRIGGER;
		case BTN_TR2: return MTY_CBUTTON_RIGHT_TRIGGER;
		case BTN_SELECT: return MTY_CBUTTON_BACK;
		case BTN_START: return MTY_CBUTTON_START;
		case BTN_MODE: return MTY_CBUTTON_GUIDE;
		case BTN_THUMBL: return MTY_CBUTTON_LEFT_THUMB;
		case BTN_THUMBR: return MTY_CBUTTON_RIGHT_THUMB;

		// Gamepad Unknown
		case BTN_C: return MTY_CBUTTON_TOUCHPAD;
		case BTN_Z: return MTY_CBUTTON_TOUCHPAD + 1;

		// Joystick
		case BTN_TRIGGER: return MTY_CBUTTON_X;
		case BTN_THUMB: return MTY_CBUTTON_A;
		case BTN_THUMB2: return MTY_CBUTTON_B;
		case BTN_TOP: return MTY_CBUTTON_Y;
		case BTN_TOP2: return MTY_CBUTTON_LEFT_SHOULDER;
		case BTN_PINKIE: return MTY_CBUTTON_RIGHT_SHOULDER;
		case BTN_BASE: return MTY_CBUTTON_LEFT_TRIGGER;
		case BTN_BASE2: return MTY_CBUTTON_RIGHT_TRIGGER;
		case BTN_BASE3: return MTY_CBUTTON_BACK;
		case BTN_BASE4: return MTY_CBUTTON_START;
		case BTN_BASE5: return MTY_CBUTTON_LEFT_THUMB;
		case BTN_BASE6: return MTY_CBUTTON_RIGHT_THUMB;

		// Joystick Unknown
		case BTN_DEAD: return MTY_CBUTTON_GUIDE;
	}

	return -1;
}

static uint16_t hid_gamepad_usage(struct hdevice *ctx, uint16_t type, const struct input_event *event)
{
	switch (type) {
		case ABS_X: return 0x30;
		case ABS_Y: return 0x31;
		case ABS_Z: return 0x33;
		case ABS_RX: return 0x32;
		case ABS_RY: return 0x35;
		case ABS_RZ: return 0x34;
	}

	return 0;
}

static uint16_t hid_joystick_usage(struct hdevice *ctx, uint16_t type, const struct input_event *event)
{
	switch (type) {
		case ABS_X: return 0x30;
		case ABS_Y: return 0x31;
		case ABS_Z: return 0x32;
		case ABS_RZ: return 0x35;
	}

	return 0;
}

static void hid_joystick_event(struct hid *ctx, int32_t fd)
{
	struct hdevice *hdev = MTY_HashGetInt(ctx->devices_rev, fd);
	if (!hdev)
		return;

	MTY_ControllerEvent *c = &hdev->state;
	struct input_event event = {0};

	if (read(fd, &event, sizeof(struct input_event)) != sizeof(struct input_event))
		return;

	if (event.type == EV_KEY) {
		if (event.code >= 0x130 && event.code < 0x140)
			hdev->gamepad = true;

		MTY_CButton cb = hid_button(event.code);

		if (cb >= 0) {
			if (cb >= MTY_CBUTTON_MAX)
				return;

			c->buttons[cb] = event.value ? true : false;
			ctx->report(hdev, NULL, 0, ctx->opaque);
		}

	} else if (event.type == EV_ABS) {
		hid_set_hat(hdev, event.code, &event);

		uint16_t usage = hdev->gamepad ? hid_gamepad_usage(hdev, event.code, &event) :
			hid_joystick_usage(hdev, event.code, &event);

		if (usage > 0) {
			uint8_t slot = hdev->ainfo[event.code].slot;

			// Slots will always begin at 1 since the DPAD is in a fixed position of 0
			if (slot == 0 || slot >= MTY_CVALUE_MAX)
				return;

			c->values[slot].data  = event.value;
			c->values[slot].usage  = usage;
			c->values[slot].min = hdev->ainfo[event.code].min;
			c->values[slot].max = hdev->ainfo[event.code].max;

		}
	}

	if (event.type != EV_SYN)
		ctx->report(hdev, NULL, 0, ctx->opaque);
}

static void hid_initial_scan(struct hid *ctx)
{
	struct udev_enumerate *enumerate = udev_enumerate_new(ctx->udev);
	if (!enumerate)
		return;

	udev_enumerate_add_match_subsystem(enumerate, "input");
	udev_enumerate_scan_devices(enumerate);

	struct udev_list_entry *devs = udev_enumerate_get_list_entry(enumerate);
	for (struct udev_list_entry *item = devs; item; item = udev_list_entry_get_next(item)) {
		const char *syspath = udev_list_entry_get_name(item);
		struct udev_device *dev = udev_device_new_from_syspath(ctx->udev, syspath);

		if (dev) {
			const char *devnode = udev_device_get_devnode(dev);
			if (devnode && strstr(devnode, "/event"))
				hid_device_add(ctx, devnode, syspath);
		}
	}

	udev_enumerate_unref(enumerate);
}

struct hid *mty_hid_create(HID_CONNECT connect, HID_DISCONNECT disconnect, HID_REPORT report, void *opaque)
{
	if (!udev_dl_global_init())
		return NULL;

	bool r = true;

	struct hid *ctx = MTY_Alloc(1, sizeof(struct hid));
	ctx->connect = connect;
	ctx->disconnect = disconnect;
	ctx->report = report;
	ctx->opaque = opaque;
	ctx->devices = MTY_HashCreate(0);
	ctx->devices_rev = MTY_HashCreate(0);

	for (uint8_t x = 0; x < HID_FD_MAX; x++) {
		ctx->fds[x].fd = -1;
		ctx->fds[x].events = POLLIN;
	}

	ctx->udev = udev_new();
	if (!ctx->udev) {
		r = false;
		MTY_Log("'udev_new' failed");
		goto except;
	}

	ctx->udev_monitor = udev_monitor_new_from_netlink(ctx->udev, "udev");
	if (!ctx->udev_monitor) {
		r = false;
		MTY_Log("'udev_monitor_new_from_netlink' failed");
		goto except;
	}

	int32_t e = udev_monitor_enable_receiving(ctx->udev_monitor);
	if (e < 0) {
		r = false;
		MTY_Log("'udev_monitor_enable_receiving' failed with error %d", e);
		goto except;
	}

	e = udev_monitor_filter_add_match_subsystem_devtype(ctx->udev_monitor, "input", NULL);
	if (e < 0) {
		r = false;
		MTY_Log("'udev_monitor_filter_add_match_subsystem_devtype' failed with error %d", e);
		goto except;
	}

	ctx->fds[0].fd = udev_monitor_get_fd(ctx->udev_monitor);
	if (ctx->fds[0].fd < 0) {
		r = false;
		MTY_Log("'udev_monitor_get_fd' failed with error %d", ctx->fds[0].fd);
		goto except;
	}

	except:

	if (!r)
		mty_hid_destroy(&ctx);

	return ctx;
}

struct hdevice *mty_hid_get_device_by_id(struct hid *ctx, uint32_t id)
{
	return MTY_HashGetInt(ctx->devices_rev, id);
}

void mty_hid_poll(struct hid *ctx)
{
	// Fire off an initial enumerate to populate already connected joysticks
	if (!ctx->init_scan) {
		hid_initial_scan(ctx);
		ctx->init_scan = true;
	}

	// Poll the monitor file descriptor (0) and any additional open joysticks
	int32_t e = poll(ctx->fds, HID_FD_MAX, 0);
	if (e <= 0)
		return;

	for (uint8_t x = 0; x < HID_FD_MAX; x++) {
		if (ctx->fds[x].revents & POLLIN) {
			// udev_monitor fd
			if (x == 0) {
				hid_new_device(ctx);

			// evdev event
			} else {
				hid_joystick_event(ctx, ctx->fds[x].fd);
			}
		}
	}
}

void mty_hid_destroy(struct hid **hid)
{
	if (!hid || !*hid)
		return;

	struct hid *ctx = *hid;

	if (ctx->udev_monitor)
		udev_monitor_unref(ctx->udev_monitor);

	if (ctx->udev)
		udev_unref(ctx->udev);

	for (uint8_t x = 0; x < HID_FD_MAX; x++)
		if (ctx->fds[x].fd != -1)
			close(ctx->fds[x].fd);

	MTY_HashDestroy(&ctx->devices, hid_device_destroy);
	MTY_HashDestroy(&ctx->devices_rev, NULL);

	MTY_Free(ctx);
	*hid = NULL;
}

void mty_hid_device_write(struct hdevice *ctx, const void *buf, size_t size)
{
}

bool mty_hid_device_feature(struct hdevice *ctx, void *buf, size_t size, size_t *size_out)
{
	return false;
}

void mty_hid_default_state(struct hdevice *ctx, const void *buf, size_t size, MTY_Event *evt)
{
	evt->type = MTY_EVENT_CONTROLLER;
	evt->controller = ctx->state;
}

void mty_hid_default_rumble(struct hid *ctx, uint32_t id, uint16_t low, uint16_t high)
{
	struct hdevice *hdev = MTY_HashGetInt(ctx->devices_rev, id);
	if (!hdev)
		return;

	if (!hdev->rumble)
		return;

	// evdev can only store a certain number of effects, make sure to delete them
	if (hdev->ff.id != -1) {
		ioctl(ctx->fds[hdev->slot].fd, EVIOCRMFF, hdev->ff.id);
		hdev->ff.id = -1;
	}

	hdev->ff.u.rumble.strong_magnitude = low;
	hdev->ff.u.rumble.weak_magnitude = high;

	// Upload the effect
	if (ioctl(ctx->fds[hdev->slot].fd, EVIOCSFF, &hdev->ff) != -1) {
		struct input_event ev = {0};
		ev.type = EV_FF;
		ev.code = hdev->ff.id;
		ev.value = 1;

		// Write the effect
		if (write(ctx->fds[hdev->slot].fd, &ev, sizeof(struct input_event)) == -1)
			MTY_Log("'write' failed with errno %d", errno);
	}
}

void *mty_hid_device_get_state(struct hdevice *ctx)
{
	return NULL;
}

uint16_t mty_hid_device_get_vid(struct hdevice *ctx)
{
	return ctx->vid;
}

uint16_t mty_hid_device_get_pid(struct hdevice *ctx)
{
	return ctx->pid;
}

uint32_t mty_hid_device_get_id(struct hdevice *ctx)
{
	return ctx->id;
}

uint32_t mty_hid_device_get_input_report_size(struct hdevice *ctx)
{
	return 0;
}

bool mty_hid_device_force_default(struct hdevice *ctx)
{
	return true;
}
