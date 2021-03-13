// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#pragma once

enum xbox_proto {
	XBOX_PROTO_UNKNOWN = 0,
	XBOX_PROTO_V1,
	XBOX_PROTO_V2,
};

struct xbox_state {
	enum xbox_proto proto;
	bool series_x;
	bool guide;
	bool rumble;
	MTY_Time rumble_ts;
	uint16_t low;
	uint16_t high;
};


// Rumble

static void mty_hid_xbox_rumble(struct hdevice *device, uint16_t low, uint16_t high)
{
	struct xbox_state *ctx = mty_hid_device_get_state(device);

	if (low != ctx->low || high != ctx->high) {
		ctx->low = low;
		ctx->high = high;
		ctx->rumble = true;
	}
}

static void mty_hid_xbox_do_rumble(struct hdevice *device)
{
	struct xbox_state *ctx = mty_hid_device_get_state(device);

	MTY_Time ts = MTY_GetTime();
	float diff = MTY_TimeDiff(ctx->rumble_ts, ts);
	bool non_zero = ctx->low > 0 || ctx->high > 0;

	// Xbox one seems to have an internal timeout of 3s
	if ((ctx->rumble && diff > 50.0f) || (non_zero && diff > 2000.0f)) {
		uint8_t rumble[9] = {0x03, 0x0F,
			0x00, // Left trigger motor
			0x00, // Right trigger motor
			0x00, // Left motor
			0x00, // Right motor
			0xFF,
			0x00,
			0x00,
		};

		rumble[4] = ctx->low >> 8;
		rumble[5] = ctx->high >> 8;

		mty_hid_device_write(device, rumble, 9);
		ctx->rumble_ts = ts;
		ctx->rumble = false;
	}
}


// State Reports

static void mty_hid_xbox_init(struct hdevice *device)
{
	struct xbox_state *ctx = mty_hid_device_get_state(device);
	ctx->series_x = mty_hid_device_get_pid(device) == 0x0B13;
	ctx->rumble = true;
}

static void mty_hid_xbox_state(struct hdevice *device, const void *data, size_t dsize, MTY_Event *evt)
{
	struct xbox_state *ctx = mty_hid_device_get_state(device);
	const uint8_t *d8 = data;

	if (d8[0] == 0x01) {
		evt->type = MTY_EVENT_CONTROLLER;

		MTY_ControllerEvent *c = &evt->controller;
		c->vid = mty_hid_device_get_vid(device);
		c->pid = mty_hid_device_get_pid(device);
		c->numValues = 7;
		c->numButtons = 14;
		c->type = MTY_CTYPE_XBOX;
		c->id = mty_hid_device_get_id(device);

		if (ctx->proto == XBOX_PROTO_UNKNOWN && d8[15] & 0x10)
			ctx->proto = XBOX_PROTO_V2;

		if (ctx->proto == XBOX_PROTO_V2)
			ctx->guide = d8[15] & 0x10;

		c->buttons[MTY_CBUTTON_X] = d8[14] & 0x08;
		c->buttons[MTY_CBUTTON_A] = d8[14] & 0x01;
		c->buttons[MTY_CBUTTON_B] = d8[14] & 0x02;
		c->buttons[MTY_CBUTTON_Y] = d8[14] & 0x10;
		c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = d8[14] & 0x40;
		c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = d8[14] & 0x80;
		c->buttons[MTY_CBUTTON_BACK] = ctx->series_x ? d8[15] & 0x04 : d8[16] & 0x01;
		c->buttons[MTY_CBUTTON_START] = d8[15] & 0x08;
		c->buttons[MTY_CBUTTON_LEFT_THUMB] = d8[15] & 0x20;
		c->buttons[MTY_CBUTTON_RIGHT_THUMB] = d8[15] & 0x40;
		c->buttons[MTY_CBUTTON_GUIDE] = ctx->guide;
		c->buttons[MTY_CBUTTON_TOUCHPAD] = ctx->series_x ? d8[16] & 0x01 : 0;

		c->values[MTY_CVALUE_THUMB_LX].data = *((int16_t *) (d8 + 1)) - 0x8000;
		c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
		c->values[MTY_CVALUE_THUMB_LX].min = INT16_MIN;
		c->values[MTY_CVALUE_THUMB_LX].max = INT16_MAX;

		int16_t ly = *((int16_t *) (d8 + 3)) - 0x8000;
		c->values[MTY_CVALUE_THUMB_LY].data = (int16_t) ((int32_t) (ly + 1) * -1);
		c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
		c->values[MTY_CVALUE_THUMB_LY].min = INT16_MIN;
		c->values[MTY_CVALUE_THUMB_LY].max = INT16_MAX;

		c->values[MTY_CVALUE_THUMB_RX].data = *((int16_t *) (d8 + 5)) - 0x8000;
		c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
		c->values[MTY_CVALUE_THUMB_RX].min = INT16_MIN;
		c->values[MTY_CVALUE_THUMB_RX].max = INT16_MAX;

		int16_t ry = *((int16_t *) (d8 + 7)) - 0x8000;
		c->values[MTY_CVALUE_THUMB_RY].data = (int16_t) ((int32_t) (ry + 1) * -1);
		c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
		c->values[MTY_CVALUE_THUMB_RY].min = INT16_MIN;
		c->values[MTY_CVALUE_THUMB_RY].max = INT16_MAX;

		c->values[MTY_CVALUE_TRIGGER_L].data = *((uint16_t *) (d8 + 9)) >> 2;;
		c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
		c->values[MTY_CVALUE_TRIGGER_L].min = 0;
		c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

		c->values[MTY_CVALUE_TRIGGER_R].data = *((uint16_t *) (d8 + 11)) >> 2;;
		c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
		c->values[MTY_CVALUE_TRIGGER_R].min = 0;
		c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;

		c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = c->values[MTY_CVALUE_TRIGGER_L].data > 0;
		c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = c->values[MTY_CVALUE_TRIGGER_R].data > 0;

		c->values[MTY_CVALUE_DPAD].data = d8[13] > 0 ? d8[13] - 1 : 8;
		c->values[MTY_CVALUE_DPAD].usage = 0x39;
		c->values[MTY_CVALUE_DPAD].min = 0;
		c->values[MTY_CVALUE_DPAD].max = 7;

	} else if (d8[0] == 0x02) {
		ctx->proto = XBOX_PROTO_V2;
		ctx->guide = d8[1] & 0x01;
	}

	mty_hid_xbox_do_rumble(device);
}
