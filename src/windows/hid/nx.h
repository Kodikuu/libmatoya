// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#define HID_NX_OUTPUT_MAX 64
#define HID_NX_USB_MAX    64
#define HID_NX_BT_MAX     49

#pragma pack(1)
struct nx_spi_op {
	uint32_t addr;
	uint8_t len;
};

struct nx_spi_reply {
	struct nx_spi_op op;
	uint8_t data[30];
};

struct nx_rumble {
	uint8_t data[4];
} rumble;

struct nx_output_common {
	uint8_t packetType;
	uint8_t packetNumber;
	struct nx_rumble rumble[2];
} common;

struct nx_subcommand {
	struct nx_output_common common;
	uint8_t subcommandID;
	uint8_t subcommandData[42];
};
#pragma pack()

struct nx_min_max {
	int16_t min;
	int16_t c;
	int16_t max;
};

struct nx_state {
	bool wready;
	bool hs0;
	bool hs1;
	bool hs2;
	bool vibration;
	bool calibration;
	bool calibrated;
	bool home_led;
	bool slot_led;
	bool bluetooth;
	bool mode;
	bool usb;
	bool rumble_on;
	bool rumble_set;
	uint8_t cmd;
	struct nx_min_max lx, slx;
	struct nx_min_max ly, sly;
	struct nx_min_max rx, srx;
	struct nx_min_max ry, sry;
	struct nx_rumble rumble[2];
	int64_t write_ts;
	int64_t rumble_ts;
};


// IO Helpers

static uint8_t hid_nx_incr_command(struct nx_state *ctx)
{
	uint8_t v = ctx->cmd;
	ctx->cmd = (ctx->cmd + 1) & 0x0F;

	return v;
}

static void hid_nx_write_hs(struct hid *hid, uint8_t cmd)
{
	struct nx_state *ctx = hid->driver_state;

	uint8_t buf[HID_NX_OUTPUT_MAX] = {0x80};
	buf[1] = cmd;

	hid_write(hid->name, buf, HID_NX_USB_MAX);

	ctx->write_ts = MTY_Timestamp();
	ctx->wready = false;
}

static void hid_nx_write_command(struct hid *hid, uint8_t command, const void *data, size_t size)
{
	struct nx_state *ctx = hid->driver_state;

	uint8_t buf[HID_NX_OUTPUT_MAX] = {0};
	struct nx_subcommand *out = (struct nx_subcommand *) buf;

	out->common.packetType = 0x01;
	out->common.packetNumber = hid_nx_incr_command(ctx);
	out->common.rumble[0] = ctx->rumble[0];
	out->common.rumble[1] = ctx->rumble[1];
	out->subcommandID = command;
	memcpy(out->subcommandData, data, size);

	hid_write(hid->name, buf, ctx->bluetooth ? HID_NX_BT_MAX : HID_NX_USB_MAX);

	ctx->write_ts = MTY_Timestamp();
	ctx->wready = false;
}


// Rumble

static void hid_nx_rumble_off(struct nx_rumble *rmb)
{
	rmb->data[0] = 0x00;
	rmb->data[1] = 0x01;
	rmb->data[2] = 0x40;
	rmb->data[3] = 0x40;
}

static void hid_nx_rumble_on(struct nx_rumble *rmb)
{
	rmb->data[0] = 0x74;
	rmb->data[1] = 0xBE;
	rmb->data[2] = 0xBD;
	rmb->data[3] = 0x6F;
}

static void hid_nx_rumble(struct hid *hid, bool low, bool high)
{
	struct nx_state *ctx = hid->driver_state;

	hid_nx_rumble_off(&ctx->rumble[0]);
	hid_nx_rumble_off(&ctx->rumble[1]);

	if (low)
		hid_nx_rumble_on(&ctx->rumble[0]);

	if (high)
		hid_nx_rumble_on(&ctx->rumble[1]);

	ctx->rumble_on = low || high;
	ctx->rumble_set = true;
}


// State Reports

static void hid_nx_expand_min_max(int16_t v, struct nx_min_max *mm)
{
	if (v < mm->min)
		mm->min = v;

	if (v > mm->max)
		mm->max = v;
}

static void hid_nx_full_state(struct hid *hid, const uint8_t *d, MTY_Msg *wmsg)
{
	struct nx_state *ctx = hid->driver_state;

	wmsg->type = MTY_WINDOW_MSG_CONTROLLER;

	MTY_Controller *c = &wmsg->controller;
	c->vid = (uint16_t) hid->di.hid.dwVendorId;
	c->pid = (uint16_t) hid->di.hid.dwProductId;
	c->numValues = 7;
	c->numButtons = 14;
	c->driver = hid->driver;
	c->id = hid->id;

	c->buttons[MTY_CBUTTON_X] = d[3] & 0x01;
	c->buttons[MTY_CBUTTON_A] = d[3] & 0x04;
	c->buttons[MTY_CBUTTON_B] = d[3] & 0x08;
	c->buttons[MTY_CBUTTON_Y] = d[3] & 0x02;
	c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = d[5] & 0x40;
	c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = d[3] & 0x40;
	c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = d[5] & 0x80;
	c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = d[3] & 0x80;
	c->buttons[MTY_CBUTTON_BACK] = d[4] & 0x01;
	c->buttons[MTY_CBUTTON_START] = d[4] & 0x02;
	c->buttons[MTY_CBUTTON_LEFT_THUMB] = d[4] & 0x08;
	c->buttons[MTY_CBUTTON_RIGHT_THUMB] = d[4] & 0x04;
	c->buttons[MTY_CBUTTON_GUIDE] = d[4] & 0x10;
	c->buttons[MTY_CBUTTON_TOUCHPAD] = d[4] & 0x20; // The "Capture" button

	int16_t lx = (d[6] | ((d[7] & 0x0F) << 8)) - ctx->lx.c;
	int16_t ly = ((d[7] >> 4) | (d[8] << 4) - ctx->ly.c);
	int16_t rx = (d[9] | ((d[10] & 0x0F) << 8)) - ctx->rx.c;
	int16_t ry = ((d[10] >> 4) | (d[11] << 4)) - ctx->ry.c;

	hid_nx_expand_min_max(lx, &ctx->lx);
	hid_nx_expand_min_max(ly, &ctx->ly);
	hid_nx_expand_min_max(rx, &ctx->rx);
	hid_nx_expand_min_max(ry, &ctx->ry);

	c->values[MTY_CVALUE_THUMB_LX].data = lx;
	c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
	c->values[MTY_CVALUE_THUMB_LX].min = ctx->lx.min;
	c->values[MTY_CVALUE_THUMB_LX].max = ctx->lx.max;
	hid_s_to_s16(&c->values[MTY_CVALUE_THUMB_LX]);

	c->values[MTY_CVALUE_THUMB_LY].data = ly;
	c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
	c->values[MTY_CVALUE_THUMB_LY].min = ctx->ly.min;
	c->values[MTY_CVALUE_THUMB_LY].max = ctx->ly.max;
	hid_s_to_s16(&c->values[MTY_CVALUE_THUMB_LY]);

	c->values[MTY_CVALUE_THUMB_RX].data = rx;
	c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
	c->values[MTY_CVALUE_THUMB_RX].min = ctx->rx.min;
	c->values[MTY_CVALUE_THUMB_RX].max = ctx->rx.max;
	hid_s_to_s16(&c->values[MTY_CVALUE_THUMB_RX]);

	c->values[MTY_CVALUE_THUMB_RY].data = ry;
	c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
	c->values[MTY_CVALUE_THUMB_RY].min = ctx->ry.min;
	c->values[MTY_CVALUE_THUMB_RY].max = ctx->ry.max;
	hid_s_to_s16(&c->values[MTY_CVALUE_THUMB_RY]);

	c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
	c->values[MTY_CVALUE_TRIGGER_L].data = c->buttons[MTY_CBUTTON_LEFT_TRIGGER] ? UINT8_MAX : 0;
	c->values[MTY_CVALUE_TRIGGER_L].min = 0;
	c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

	c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
	c->values[MTY_CVALUE_TRIGGER_R].data = c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] ? UINT8_MAX : 0;
	c->values[MTY_CVALUE_TRIGGER_R].min = 0;
	c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;

	bool up = d[5] & 0x02;
	bool down = d[5] & 0x01;
	bool left = d[5] & 0x08;
	bool right = d[5] & 0x04;

	c->values[MTY_CVALUE_DPAD].data = (up && right) ? 1 : (right && down) ? 3 :
		(down && left) ? 5 : (left && up) ? 7 : up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;
	c->values[MTY_CVALUE_DPAD].usage = 0x39;
	c->values[MTY_CVALUE_DPAD].min = 0;
	c->values[MTY_CVALUE_DPAD].max = 7;
}

static void hid_nx_simple_state(struct hid *hid, const uint8_t *d, MTY_Msg *wmsg)
{
	struct nx_state *ctx = hid->driver_state;

	wmsg->type = MTY_WINDOW_MSG_CONTROLLER;

	MTY_Controller *c = &wmsg->controller;
	c->vid = (uint16_t) hid->di.hid.dwVendorId;
	c->pid = (uint16_t) hid->di.hid.dwProductId;
	c->numValues = 7;
	c->numButtons = 14;
	c->driver = hid->driver;
	c->id = hid->id;

	c->buttons[MTY_CBUTTON_X] = d[1] & 0x04;
	c->buttons[MTY_CBUTTON_A] = d[1] & 0x01;
	c->buttons[MTY_CBUTTON_B] = d[1] & 0x02;
	c->buttons[MTY_CBUTTON_Y] = d[1] & 0x08;
	c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = d[1] & 0x10;
	c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = d[1] & 0x20;
	c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = d[1] & 0x40;
	c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = d[1] & 0x80;
	c->buttons[MTY_CBUTTON_BACK] = d[2] & 0x01;
	c->buttons[MTY_CBUTTON_START] = d[2] & 0x02;
	c->buttons[MTY_CBUTTON_LEFT_THUMB] = d[2] & 0x04;
	c->buttons[MTY_CBUTTON_RIGHT_THUMB] = d[2] & 0x08;
	c->buttons[MTY_CBUTTON_GUIDE] = d[2] & 0x20;
	c->buttons[MTY_CBUTTON_TOUCHPAD] = d[2] & 0x10;

	int16_t lx = (d[4] | (d[5] << 8)) - ctx->slx.c;
	int16_t ly = (d[6] | (d[7] << 8)) - ctx->sly.c;
	int16_t rx = (d[8] | (d[9] << 8)) - ctx->srx.c;
	int16_t ry = (d[10] | (d[11] << 8)) - ctx->sry.c;

	hid_nx_expand_min_max(lx, &ctx->slx);
	hid_nx_expand_min_max(ly, &ctx->sly);
	hid_nx_expand_min_max(rx, &ctx->srx);
	hid_nx_expand_min_max(ry, &ctx->sry);

	c->values[MTY_CVALUE_THUMB_LX].data = lx;
	c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
	c->values[MTY_CVALUE_THUMB_LX].min = ctx->slx.min;
	c->values[MTY_CVALUE_THUMB_LX].max = ctx->slx.max;
	hid_s_to_s16(&c->values[MTY_CVALUE_THUMB_LX]);

	c->values[MTY_CVALUE_THUMB_LY].data = ly;
	c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
	c->values[MTY_CVALUE_THUMB_LY].min = ctx->sly.min;
	c->values[MTY_CVALUE_THUMB_LY].max = ctx->sly.max;
	hid_s_to_s16(&c->values[MTY_CVALUE_THUMB_LY]);

	c->values[MTY_CVALUE_THUMB_RX].data = rx;
	c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
	c->values[MTY_CVALUE_THUMB_RX].min = ctx->srx.min;
	c->values[MTY_CVALUE_THUMB_RX].max = ctx->srx.max;
	hid_s_to_s16(&c->values[MTY_CVALUE_THUMB_RX]);

	c->values[MTY_CVALUE_THUMB_RY].data = ry;
	c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
	c->values[MTY_CVALUE_THUMB_RY].min = ctx->sry.min;
	c->values[MTY_CVALUE_THUMB_RY].max = ctx->sry.max;
	hid_s_to_s16(&c->values[MTY_CVALUE_THUMB_RY]);

	c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
	c->values[MTY_CVALUE_TRIGGER_L].data = c->buttons[MTY_CBUTTON_LEFT_TRIGGER] ? UINT8_MAX : 0;
	c->values[MTY_CVALUE_TRIGGER_L].min = 0;
	c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

	c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
	c->values[MTY_CVALUE_TRIGGER_R].data = c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] ? UINT8_MAX : 0;
	c->values[MTY_CVALUE_TRIGGER_R].min = 0;
	c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;

	c->values[MTY_CVALUE_DPAD].data = d[3];
	c->values[MTY_CVALUE_DPAD].usage = 0x39;
	c->values[MTY_CVALUE_DPAD].min = 0;
	c->values[MTY_CVALUE_DPAD].max = 7;
}


// Full Stick Calibration Data

static void hid_nx_parse_pair(const uint8_t *data, uint8_t offset, uint16_t *v0, uint16_t *v1)
{
	*v0 = ((data[offset + 1] << 8) & 0xF00) | data[offset];
	*v1 = (data[offset + 2] << 4) | (data[offset + 1] >> 4);

	if (*v0 == 0x0FFF)
		*v0 = 0;

	if (*v1 == 0x0FFF)
		*v1 = 0;
}

static void hid_nx_parse_stick(const struct nx_spi_reply *spi, uint8_t min_offset, uint8_t c_offset, uint8_t max_offset,
	struct nx_min_max *x, struct nx_min_max *y)
{
	uint16_t xmin = 0, ymin = 0;
	hid_nx_parse_pair(spi->data, min_offset, &xmin, &ymin);

	uint16_t xc = 0, yc = 0;
	hid_nx_parse_pair(spi->data, c_offset, &xc, &yc);

	uint16_t xmax = 0, ymax = 0;
	hid_nx_parse_pair(spi->data, max_offset, &xmax, &ymax);

	x->min = (int16_t) lrint((float) (xc - xmin - xc) * 0.9f);
	x->c = xc;
	x->max = (int16_t) lrint((float) (xc + xmax - xc) * 0.9f);

	y->min = (int16_t) lrint((float) (yc - ymin - yc) * 0.9f);
	y->c = yc;
	y->max = (int16_t) lrint((float) (yc + ymax - yc) * 0.9f);
}


// IO State Machine

static void hid_nx_init(struct hid *hid)
{
	struct nx_state *ctx = hid->driver_state;

	// Neutral rumble
	hid_nx_rumble_off(&ctx->rumble[0]);
	hid_nx_rumble_off(&ctx->rumble[1]);

	// Simple calibration
	ctx->slx.c = ctx->sly.c = ctx->srx.c = ctx->sry.c = 0x8000;
	ctx->slx.min = ctx->sly.min = ctx->srx.min = ctx->sry.min = INT16_MIN / 2;
	ctx->slx.max = ctx->sly.max = ctx->srx.max = ctx->sry.max = INT16_MAX / 2;

	// USB init packet (begins handshake)
	hid_nx_write_hs(hid, 0x02);
}

static void hid_nx_state_machine(struct hid *hid)
{
	struct nx_state *ctx = hid->driver_state;

	int64_t now = MTY_Timestamp();
	bool timeout = MTY_TimeDiff(ctx->write_ts, now) > 500.0f;

	// USB Handshake is used to decide whether we're dealing with bluetooth or not
	if (!ctx->usb && !ctx->bluetooth && timeout)
		ctx->bluetooth = true;

	if (timeout)
		ctx->wready = true;

	// Once we know if we're USB or Bluetooth, start the state machine
	if ((ctx->usb || ctx->bluetooth)) {
		// USB Only
		if (ctx->usb) {
			// Set speed to 3mbit, may timeout on certain devices
			if (ctx->wready && !ctx->hs0) {
				hid_nx_write_hs(hid, 0x03);
				ctx->hs0 = true;
			}

			// Send an additional handshake packet after baud rate
			if (ctx->wready && !ctx->hs1) {
				hid_nx_write_hs(hid, 0x02);
				ctx->hs1 = true;
			}

			// Force USB
			if (ctx->wready && !ctx->hs2) {
				hid_nx_write_hs(hid, 0x04);
				ctx->wready = true; // There is no response to this
				ctx->hs2 = true;
			}
		}

		// Calibration (SPI Flash Read)
		if (ctx->wready && !ctx->calibration) {
			struct nx_spi_op op = {0x603D, 18};
			hid_nx_write_command(hid, 0x10, &op, sizeof(struct nx_spi_op));
			ctx->calibration = true;
		}

		// Enable vibration
		if (ctx->wready && !ctx->vibration) {
			uint8_t enabled = 1;
			hid_nx_write_command(hid, 0x48, &enabled, 1);
			ctx->vibration = true;
		}

		// Set to full / simple report mode
		if (ctx->wready && !ctx->mode) {
			uint8_t mode = ctx->bluetooth ? 0x3F : 0x30;
			hid_nx_write_command(hid, 0x03, &mode, 1);
			ctx->mode = true;
		}

		// Home LED
		if (ctx->wready && !ctx->home_led) {
			uint8_t buf[4] = {0x01, 0xF0, 0xF0, 0x00};
			hid_nx_write_command(hid, 0x38, buf, 4);
			ctx->home_led = true;
		}

		// Player Slot LED
		if (ctx->wready && !ctx->slot_led) {
			uint8_t data = 0x01; // Always "Player 1" TODO? Generate a real player ID
			hid_nx_write_command(hid, 0x30, &data, 1);
			ctx->slot_led = true;
		}

		// Periodically refresh rumble
		if (ctx->wready && ((ctx->rumble_on && MTY_TimeDiff(ctx->rumble_ts, now) > 500.0f) || ctx->rumble_set)) {
			uint8_t enabled = 1;
			hid_nx_write_command(hid, 0x48, &enabled, 1);
			ctx->rumble_ts = now;
			ctx->rumble_set = false;
		}
	}
}

static void hid_nx_state(struct hid *hid, const void *data, ULONG dsize, MTY_Msg *wmsg)
{
	struct nx_state *ctx = hid->driver_state;
	const uint8_t *d = data;

	// State (Full)
	if (d[0] == 0x30 && ctx->calibrated) {
		hid_nx_full_state(hid, d, wmsg);

	// State (Simple)
	} else if (d[0] == 0x3F && ctx->calibrated) {
		hid_nx_simple_state(hid, d, wmsg);

	// USB Handshake Response
	} else if (d[0] == 0x81 && (d[1] == 0x02 || d[1] == 0x03 || d[2] == 0x04)) {
		ctx->wready = true;
		ctx->usb = true;

	// Subcommand Response
	} else if (d[0] == 0x21) {
		uint8_t subcommand = d[14];
		const uint8_t *payload = d + 15;
		ctx->wready = true;

		switch (subcommand) {
			// SPI Flash Read
			case 0x10: {
				const struct nx_spi_reply *spi = (const struct nx_spi_reply *) payload;
				hid_nx_parse_stick(spi, 6, 3, 0, &ctx->lx, &ctx->ly);
				hid_nx_parse_stick(spi, 12, 9, 15, &ctx->rx, &ctx->ry);
				ctx->calibrated = true;
				break;
			}
		}
	}

	hid_nx_state_machine(hid);
}
