// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define HID_STATE_MAX 1024

struct hid;
struct hdevice;

typedef void (*HID_CONNECT)(struct hdevice *device, void *opaque);
typedef void (*HID_DISCONNECT)(struct hdevice *device, void *opaque);
typedef void (*HID_REPORT)(struct hdevice *device, const void *buf, size_t size, void *opaque);

struct hid *hid_create(HID_CONNECT connect, HID_DISCONNECT disconnect, HID_REPORT report, void *opaque);
struct hdevice *hid_get_device_by_id(struct hid *ctx, uint32_t id);
void hid_destroy(struct hid **hid);
// void hid_feed_report(struct hid *ctx, void *native_device, const void *buf, size_t size);

void hid_device_write(struct hdevice *ctx, const void *buf, size_t size);
bool hid_device_feature(struct hdevice *ctx, void *buf, size_t size, size_t *size_out);
void *hid_device_get_state(struct hdevice *ctx);
uint16_t hid_device_get_vid(struct hdevice *ctx);
uint16_t hid_device_get_pid(struct hdevice *ctx);
uint32_t hid_device_get_id(struct hdevice *ctx);