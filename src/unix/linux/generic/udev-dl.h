// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once


// Interface

struct udev;
struct udev_enumerate;
struct udev_list_entry;
struct udev_monitor;
struct udev_device;

static struct udev *(*udev_new)(void);
static struct udev *(*udev_unref)(struct udev *udev);
static struct udev_monitor *(*udev_monitor_new_from_netlink)(struct udev *udev, const char *name);
static int (*udev_monitor_enable_receiving)(struct udev_monitor *udev_monitor);
static int (*udev_monitor_filter_add_match_subsystem_devtype)(struct udev_monitor *udev_monitor,
	const char *subsystem, const char *devtype);
static struct udev_monitor *(*udev_monitor_unref)(struct udev_monitor *udev_monitor);
static int (*udev_monitor_get_fd)(struct udev_monitor *udev_monitor);
static struct udev_device *(*udev_monitor_receive_device)(struct udev_monitor *udev_monitor);
static struct udev_device *(*udev_device_new_from_syspath)(struct udev *udev, const char *syspath);
static const char *(*udev_device_get_syspath)(struct udev_device *udev_device);
static const char *(*udev_device_get_action)(struct udev_device *udev_device);
static const char *(*udev_device_get_devnode)(struct udev_device *udev_device);
static struct udev_device *(*udev_device_unref)(struct udev_device *udev_device);
static struct udev_enumerate *(*udev_enumerate_new)(struct udev *udev);
static int (*udev_enumerate_add_match_subsystem)(struct udev_enumerate *udev_enumerate, const char *subsystem);
static int (*udev_enumerate_scan_devices)(struct udev_enumerate *udev_enumerate);
static struct udev_list_entry *(*udev_enumerate_get_list_entry)(struct udev_enumerate *udev_enumerate);
static struct udev_enumerate *(*udev_enumerate_unref)(struct udev_enumerate *udev_enumerate);
static struct udev_list_entry *(*udev_list_entry_get_next)(struct udev_list_entry *list_entry);
static const char *(*udev_list_entry_get_name)(struct udev_list_entry *list_entry);


// Runtime open

static MTY_Atomic32 UDEV_DL_LOCK;
static MTY_SO *UDEV_DL_SO;
static bool UDEV_DL_INIT;

static void __attribute__((destructor)) udev_dl_global_destroy(void)
{
	MTY_GlobalLock(&UDEV_DL_LOCK);

	MTY_SOUnload(&UDEV_DL_SO);
	UDEV_DL_INIT = false;

	MTY_GlobalUnlock(&UDEV_DL_LOCK);
}

static bool udev_dl_global_init(void)
{
	MTY_GlobalLock(&UDEV_DL_LOCK);

	if (!UDEV_DL_INIT) {
		bool r = true;

		UDEV_DL_SO = MTY_SOLoad("libudev.so.1");
		if (!UDEV_DL_SO) {
			r = false;
			goto except;
		}

		#define UDEV_DL_LOAD_SYM(so, name) \
			name = MTY_SOGetSymbol(so, #name); \
			if (!name) {r = false; goto except;}

		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_new);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_unref);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_monitor_new_from_netlink);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_monitor_enable_receiving);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_monitor_filter_add_match_subsystem_devtype);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_monitor_unref);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_monitor_get_fd);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_monitor_receive_device);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_device_new_from_syspath);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_device_get_action);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_device_get_syspath);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_device_get_devnode);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_device_unref);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_enumerate_new);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_enumerate_add_match_subsystem);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_enumerate_scan_devices);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_enumerate_get_list_entry);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_enumerate_unref);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_list_entry_get_next);
		UDEV_DL_LOAD_SYM(UDEV_DL_SO, udev_list_entry_get_name);

		except:

		if (!r)
			udev_dl_global_destroy();

		UDEV_DL_INIT = r;
	}

	MTY_GlobalUnlock(&UDEV_DL_LOCK);

	return UDEV_DL_INIT;
}
