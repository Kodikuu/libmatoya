// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

struct ps4 {
	uint8_t leftX;
	uint8_t leftY;
	uint8_t rightX;
	uint8_t rightY;
	uint8_t buttons[3];
	uint8_t triggerLeft;
	uint8_t triggerRight;
	uint8_t _pad0[3];
	int16_t gyroX;
	int16_t gyroY;
	int16_t gyroZ;
	int16_t accelX;
	int16_t accelY;
	int16_t accelZ;
	uint8_t _pad1[5];
	uint8_t batteryLevel;
	uint8_t _pad2[4];
	uint8_t trackpadCounter1;
	uint8_t trackpadData1[3];
	uint8_t trackpadCounter2;
	uint8_t trackpadData2[3];
};

struct ps4_effects {
	uint8_t rumbleRight;
	uint8_t rumbleLeft;
	uint8_t ledRed;
	uint8_t ledGreen;
	uint8_t ledBlue;
	uint8_t ledDelayOn;
	uint8_t ledDelayOff;
	uint8_t _pad[8];
	uint8_t volumeLeft;
	uint8_t volumeRight;
	uint8_t volumeMic;
	uint8_t volumeSpeaker;
};

struct ps4_state {
	bool bluetooth;
};


// Rumble

static uint32_t mty_hid_ps4_crc32_for_byte(uint32_t r)
{
	for (int32_t i = 0; i < 8; i++)
		r = (r & 1 ? 0 : (uint32_t) 0xEDB88320L) ^ r >> 1;

	return r ^ (uint32_t) 0xFF000000L;
}

static uint32_t mty_hid_ps4_crc32(uint32_t crc, const void *data, int32_t count)
{
	for (int32_t i = 0; i < count; i++)
		crc = mty_hid_ps4_crc32_for_byte((uint8_t) crc ^ ((const uint8_t *) data)[i]) ^ crc >> 8;

	return crc;
}

static void mty_hid_ps4_rumble(struct hdevice *device, uint16_t low, uint16_t high)
{
	struct ps4_state *ctx = mty_hid_device_get_state(device);

	uint8_t data[78] = {0x05, 0x07};
	uint32_t size = 32;
	size_t offset = 4;

	if (ctx->bluetooth) {
		data[0] = 0x11;
		data[1] = 0xC4;
		data[3] = 0x03;

		size = 78;
		offset = 6;
	}

	struct ps4_effects *effects = (struct ps4_effects *) (data + offset);
	effects->rumbleLeft = low >> 8;
	effects->rumbleRight = high >> 8;

	effects->ledRed = 0x00;
	effects->ledGreen = 0x00;
	effects->ledBlue = 0x40;

	if (ctx->bluetooth) {
		uint8_t hdr = 0xA2;
		uint32_t crc = mty_hid_ps4_crc32(0, &hdr, 1);
		crc = mty_hid_ps4_crc32(crc, data, size - 4);
		memcpy(data + size - 4, &crc, 4);
	}

	mty_hid_device_write(device, data, size);
}


// State Reports

static void mty_hid_ps4_init(struct hdevice *device)
{
	struct ps4_state *ctx = mty_hid_device_get_state(device);

	// Bluetooth detection, use the Serial Number feature report
	size_t n = 0;
	uint8_t data[65] = {0x12};
	uint8_t dataz[65] = {0};
	ctx->bluetooth = !mty_hid_device_feature(device, data, 65, &n) && !memcmp(data + 1, dataz + 1, 64);

	// For lights
	mty_hid_ps4_rumble(device, 0, 0);
}

static void mty_hid_ps4_state(struct hdevice *device, const void *data, size_t dsize, MTY_Event *evt)
{
	const uint8_t *d8 = data;
	const struct ps4 *state = NULL;

	// Wired
	if (d8[0] == 0x01) {
		state = (struct ps4 *) (d8 + 1);

	// Bluetooth
	} else if (d8[0] == 0x11) {
		state = (struct ps4 *) (d8 + 3);

	} else {
		return;
	}

	evt->type = MTY_EVENT_CONTROLLER;

	MTY_ControllerEvent *c = &evt->controller;
	c->vid = mty_hid_device_get_vid(device);
	c->pid = mty_hid_device_get_pid(device);
	c->numValues = 7;
	c->numButtons = 14;
	c->type = MTY_CTYPE_PS4;
	c->id = mty_hid_device_get_id(device);

	c->buttons[MTY_CBUTTON_X] = state->buttons[0] & 0x10;
	c->buttons[MTY_CBUTTON_A] = state->buttons[0] & 0x20;
	c->buttons[MTY_CBUTTON_B] = state->buttons[0] & 0x40;
	c->buttons[MTY_CBUTTON_Y] = state->buttons[0] & 0x80;
	c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = state->buttons[1] & 0x01;
	c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = state->buttons[1] & 0x02;
	c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = state->buttons[1] & 0x04;
	c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = state->buttons[1] & 0x08;
	c->buttons[MTY_CBUTTON_BACK] = state->buttons[1] & 0x10;
	c->buttons[MTY_CBUTTON_START] = state->buttons[1] & 0x20;
	c->buttons[MTY_CBUTTON_LEFT_THUMB] = state->buttons[1] & 0x40;
	c->buttons[MTY_CBUTTON_RIGHT_THUMB] = state->buttons[1] & 0x80;
	c->buttons[MTY_CBUTTON_GUIDE] = state->buttons[2] & 0x01;
	c->buttons[MTY_CBUTTON_TOUCHPAD] = state->buttons[2] & 0x02;

	c->values[MTY_CVALUE_THUMB_LX].data = state->leftX;
	c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
	c->values[MTY_CVALUE_THUMB_LX].min = 0;
	c->values[MTY_CVALUE_THUMB_LX].max = UINT8_MAX;
	mty_hid_u_to_s16(&c->values[MTY_CVALUE_THUMB_LX], false);

	c->values[MTY_CVALUE_THUMB_LY].data = state->leftY;
	c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
	c->values[MTY_CVALUE_THUMB_LY].min = 0;
	c->values[MTY_CVALUE_THUMB_LY].max = UINT8_MAX;
	mty_hid_u_to_s16(&c->values[MTY_CVALUE_THUMB_LY], true);

	c->values[MTY_CVALUE_THUMB_RX].data = state->rightX;
	c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
	c->values[MTY_CVALUE_THUMB_RX].min = 0;
	c->values[MTY_CVALUE_THUMB_RX].max = UINT8_MAX;
	mty_hid_u_to_s16(&c->values[MTY_CVALUE_THUMB_RX], false);

	c->values[MTY_CVALUE_THUMB_RY].data = state->rightY;
	c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
	c->values[MTY_CVALUE_THUMB_RY].min = 0;
	c->values[MTY_CVALUE_THUMB_RY].max = UINT8_MAX;
	mty_hid_u_to_s16(&c->values[MTY_CVALUE_THUMB_RY], true);

	c->values[MTY_CVALUE_TRIGGER_L].data = state->triggerLeft;
	c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
	c->values[MTY_CVALUE_TRIGGER_L].min = 0;
	c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

	c->values[MTY_CVALUE_TRIGGER_R].data = state->triggerRight;
	c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
	c->values[MTY_CVALUE_TRIGGER_R].min = 0;
	c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;

	c->values[MTY_CVALUE_DPAD].data = state->buttons[0] & 0x0F;
	c->values[MTY_CVALUE_DPAD].usage = 0x39;
	c->values[MTY_CVALUE_DPAD].min = 0;
	c->values[MTY_CVALUE_DPAD].max = 7;
}
