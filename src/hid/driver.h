// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#pragma once

#include "hid.h"


// Helpers

static void mty_hid_u_to_s16(MTY_Value *v, bool invert)
{
	if (v->max == 0 && v->min == 0)
		return;

	float data = v->data;
	float max = v->max;

	if (v->min < 0) {
		data += (float) abs(v->min);
		max += (float) abs(v->min);

	} else if (v->min > 0) {
		data -= (float) v->min;
		max -= (float) v->min;
	}

	int32_t d = lrint((data / max) * (float) UINT16_MAX);
	v->data = (int16_t) (invert ? -(d - INT16_MAX) : (d - INT16_MAX - 1));
	v->min = INT16_MIN;
	v->max = INT16_MAX;
}

static void mty_hid_s_to_s16(MTY_Value *v)
{
	if (v->data < 0)
		v->data = (int16_t) lrint((float) v->data / (float) v->min * (float) INT16_MIN);

	if (v->data > 0)
		v->data = (int16_t) lrint((float) v->data / (float) v->max * (float) INT16_MAX);

	v->min = INT16_MIN;
	v->max = INT16_MAX;
}

static void mty_hid_u_to_u8(MTY_Value *v)
{
	if (v->max == 0 && v->min == 0)
		return;

	float data = v->data;
	float max = v->max;

	if (v->min < 0) {
		data += (float) abs(v->min);
		max += (float) abs(v->min);

	} else if (v->min > 0) {
		data -= (float) v->min;
		max -= (float) v->min;
	}

	int32_t d = lrint((data / max) * (float) UINT8_MAX);
	v->data = (int16_t) d;
	v->min = 0;
	v->max = UINT8_MAX;
}


// Default mapping

static bool mty_hid_default_swap_value(MTY_Value *values, MTY_CValue a, MTY_CValue b)
{
	if (a != b && values[a].usage != values[b].usage) {
		MTY_Value tmp = values[b];
		values[b] = values[a];
		values[a] = tmp;
		return true;
	}

	return false;
}

static void mty_hid_default_move_value(MTY_ControllerEvent *c, MTY_Value *v, uint16_t usage,
	int16_t data, int16_t min, int16_t max)
{
	if (v->usage != usage && c->numValues < MTY_CVALUE_MAX) {
		if (v->usage != 0x00) {
			MTY_Value *end = &c->values[c->numValues++];
			*end = *v;
		}

		v->usage = usage;
		v->data = data;
		v->min = min;
		v->max = max;

		// Fill in trigger values if they do not exist
		if (v->usage == 0x33)
			v->data = c->buttons[MTY_CBUTTON_LEFT_TRIGGER] ? UINT8_MAX : 0;

		if (v->usage == 0x34)
			v->data = c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] ? UINT8_MAX : 0;
	}
}

static void mty_hid_default_map_values(MTY_ControllerEvent *c)
{
	// Make sure there is enough room for the standard CVALUEs
	if (c->numValues < MTY_CVALUE_DPAD + 1)
		c->numValues = MTY_CVALUE_DPAD + 1;

	// Swap positions
	for (uint8_t x = 0; x < c->numValues; x++) {
		retry:

		switch (c->values[x].usage) {
			case 0x30: // X -> Left Stick X
				if (mty_hid_default_swap_value(c->values, x, MTY_CVALUE_THUMB_LX))
					goto retry;
				break;
			case 0x31: // Y -> Left Stick Y
				if (mty_hid_default_swap_value(c->values, x, MTY_CVALUE_THUMB_LY))
					goto retry;
				break;
			case 0x32: // Z -> Right Stick X
				if (mty_hid_default_swap_value(c->values, x, MTY_CVALUE_THUMB_RX))
					goto retry;
				break;
			case 0x35: // Rz -> Right Stick Y
				if (mty_hid_default_swap_value(c->values, x, MTY_CVALUE_THUMB_RY))
					goto retry;
				break;
			case 0x33: // Rx -> Left Trigger
				if (mty_hid_default_swap_value(c->values, x, MTY_CVALUE_TRIGGER_L))
					goto retry;
				break;
			case 0x34: // Ry -> Right Trigger
				if (mty_hid_default_swap_value(c->values, x, MTY_CVALUE_TRIGGER_R))
					goto retry;
				break;
			case 0x39: // Hat -> DPAD
				if (mty_hid_default_swap_value(c->values, x, MTY_CVALUE_DPAD))
					goto retry;
				break;
		}
	}

	// Move values that are not in the right positions to the end
	for (uint8_t x = 0; x < c->numValues; x++) {
		MTY_Value *v = &c->values[x];

		switch (x) {
			case MTY_CVALUE_THUMB_LX:  mty_hid_default_move_value(c, v, 0x30, 0, INT16_MIN, INT16_MAX); break;
			case MTY_CVALUE_THUMB_LY:  mty_hid_default_move_value(c, v, 0x31, 0, INT16_MIN, INT16_MAX); break;
			case MTY_CVALUE_THUMB_RX:  mty_hid_default_move_value(c, v, 0x32, 0, INT16_MIN, INT16_MAX); break;
			case MTY_CVALUE_THUMB_RY:  mty_hid_default_move_value(c, v, 0x35, 0, INT16_MIN, INT16_MAX); break;
			case MTY_CVALUE_TRIGGER_L: mty_hid_default_move_value(c, v, 0x33, 0, 0, UINT8_MAX); break;
			case MTY_CVALUE_TRIGGER_R: mty_hid_default_move_value(c, v, 0x34, 0, 0, UINT8_MAX); break;
			case MTY_CVALUE_DPAD:      mty_hid_default_move_value(c, v, 0x39, 8, 0, 7); break;
		}
	}

	// Convert to int16_t
	for (uint8_t x = 0; x < c->numValues; x++) {
		MTY_Value *v = &c->values[x];

		switch (v->usage) {
			case 0x30: // X -> Left Stick X
			case 0x32: // Z -> Right Stick X
				mty_hid_u_to_s16(v, false);
				break;
			case 0x31: // Y -> Left Stick Y
			case 0x35: // Rz -> Right Stick Y
				mty_hid_u_to_s16(v, true);
				break;
			case 0x33: // Rx -> Left Trigger
			case 0x34: // Ry -> Right Trigger
			case 0x36: // Slider
			case 0x37: // Dial
			case 0x38: // Wheel
				mty_hid_u_to_u8(v);
				break;
		}
	}
}


// Drivers

#include "ps4.h"
#include "ps5.h"
#include "nx.h"
#include "xbox.h"
#include "xboxw.h"

static MTY_CType mty_hid_driver(struct hdevice *device)
{
	if (mty_hid_device_force_default(device))
		return MTY_CTYPE_DEFAULT;

	uint16_t vid = mty_hid_device_get_vid(device);
	uint16_t pid = mty_hid_device_get_pid(device);
	uint32_t id = ((uint32_t) vid << 16) | pid;

	switch (id) {
		// Switch
		case 0x057E2009: // Nintendo Switch Pro
		case 0x057E2006: // Nintendo Switch Joycon
		case 0x057E2007: // Nintendo Switch Joycon
		case 0x057E2017: // Nintendo Switch SNES Controller
			return MTY_CTYPE_SWITCH;

		// PS4
		case 0x054C05C4: // Sony DualShock 4 Gen1
		case 0x054C09CC: // Sony DualShock 4 Gen2
		case 0x054C0BA0: // Sony PS4 Controller (Wireless dongle)
			return MTY_CTYPE_PS4;

		// PS5
		case 0x054C0CE6: // Sony DualSense
			return MTY_CTYPE_PS5;

		// Xbox
		case 0x045E02E0: // Microsoft X-Box One S pad (Bluetooth)
		case 0x045E02FD: // Microsoft X-Box One S pad (Bluetooth)
		case 0x045E0B05: // Microsoft X-Box One Elite Series 2 pad (Bluetooth)
		case 0x045E0B13: // Microsoft X-Box Series X (Bluetooth)
			return MTY_CTYPE_XBOX;

		// Xbox Wired
		case 0x045E028E: // Microsoft XBox 360
		case 0x045E028F: // Microsoft XBox 360 v2
		case 0x045E02A1: // Microsoft XBox 360
		case 0x045E0291: // Microsoft XBox 360 Wireless Dongle
		case 0x045E0719: // Microsoft XBox 360 Wireless Dongle
		case 0x045E02A0: // Microsoft Xbox 360 Big Button IR
		case 0x045E02DD: // Microsoft XBox One
		case 0x044FB326: // Microsoft XBox One Firmware 2015
		case 0x045E02E3: // Microsoft XBox One Elite
		case 0x045E02FF: // Microsoft XBox One Elite
		case 0x045E02EA: // Microsoft XBox One S
		case 0x046DC21D: // Logitech F310
		case 0x0E6F02A0: // PDP Xbox One
			return MTY_CTYPE_XBOXW;
	}

	return MTY_CTYPE_DEFAULT;
}

static void mty_hid_driver_init(struct hdevice *device)
{
	switch (mty_hid_driver(device)) {
		case MTY_CTYPE_SWITCH:
			mty_hid_nx_init(device);
			break;
		case MTY_CTYPE_PS4:
			mty_hid_ps4_init(device);
			break;
		case MTY_CTYPE_XBOX:
			mty_hid_xbox_init(device);
			break;
	}
}

static void mty_hid_driver_state(struct hdevice *device, const void *buf, size_t size, MTY_Event *evt)
{
	switch (mty_hid_driver(device)) {
		case MTY_CTYPE_SWITCH:
			mty_hid_nx_state(device, buf, size, evt);
			break;
		case MTY_CTYPE_PS4:
			mty_hid_ps4_state(device, buf, size, evt);
			break;
		case MTY_CTYPE_PS5:
			mty_hid_ps5_state(device, buf, size, evt);
			break;
		case MTY_CTYPE_XBOX:
			mty_hid_xbox_state(device, buf, size, evt);
			break;
		case MTY_CTYPE_XBOXW:
			mty_hid_xboxw_state(device, buf, size, evt);
			break;
		case MTY_CTYPE_DEFAULT:
			mty_hid_default_state(device, buf, size, evt);
			mty_hid_default_map_values(&evt->controller);
			break;
	}
}

static void mty_hid_driver_rumble(struct hid *hid, uint32_t id, uint16_t low, uint16_t high)
{
	struct hdevice *device = mty_hid_get_device_by_id(hid, id);
	if (!device)
		return;

	switch (mty_hid_driver(device)) {
		case MTY_CTYPE_SWITCH:
			mty_hid_nx_rumble(device, low > 0, high > 0);
			break;
		case MTY_CTYPE_PS4:
			mty_hid_ps4_rumble(device, low, high);
			break;
		case MTY_CTYPE_PS5:
			mty_hid_ps5_rumble(device, low, high);
			break;
		case MTY_CTYPE_XBOX:
			mty_hid_xbox_rumble(device, low, high);
			break;
		case MTY_CTYPE_DEFAULT:
			mty_hid_default_rumble(hid, id, low, high);
			break;
	}
}
