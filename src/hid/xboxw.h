// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

static void mty_hid_xboxw_state(struct hdevice *device, const void *data, size_t size, MTY_Event *evt)
{
	const uint8_t *d = data;

	evt->type = MTY_EVENT_CONTROLLER;

	MTY_ControllerEvent *c = &evt->controller;
	c->vid = mty_hid_device_get_vid(device);
	c->pid = mty_hid_device_get_pid(device);
	c->numValues = 7;
	c->numButtons = 13;
	c->type = MTY_CTYPE_XBOXW;
	c->id = mty_hid_device_get_id(device);

	c->buttons[MTY_CBUTTON_X] = d[3] & 0x40;
	c->buttons[MTY_CBUTTON_A] = d[3] & 0x10;
	c->buttons[MTY_CBUTTON_B] = d[3] & 0x20;
	c->buttons[MTY_CBUTTON_Y] = d[3] & 0x80;
	c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = d[3] & 0x01;
	c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = d[3] & 0x02;
	c->buttons[MTY_CBUTTON_BACK] = d[2] & 0x20;
	c->buttons[MTY_CBUTTON_START] = d[2] & 0x10;
	c->buttons[MTY_CBUTTON_LEFT_THUMB] = d[2] & 0x40;
	c->buttons[MTY_CBUTTON_RIGHT_THUMB] = d[2] & 0x80;
	c->buttons[MTY_CBUTTON_GUIDE] = d[3] & 0x04;

	c->values[MTY_CVALUE_THUMB_LX].data = *((int16_t *) (d + 6));
	c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
	c->values[MTY_CVALUE_THUMB_LX].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_LX].max = INT16_MAX;

	int16_t ly = *((int16_t *) (d + 8));
	c->values[MTY_CVALUE_THUMB_LY].data = (int16_t) ((int32_t) (ly + 1) * -1);
	c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
	c->values[MTY_CVALUE_THUMB_LY].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_LY].max = INT16_MAX;

	c->values[MTY_CVALUE_THUMB_RX].data = *((int16_t *) (d + 10));
	c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
	c->values[MTY_CVALUE_THUMB_RX].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_RX].max = INT16_MAX;

	int16_t ry = *((int16_t *) (d + 12));
	c->values[MTY_CVALUE_THUMB_RY].data = (int16_t) ((int32_t) (ry + 1) * -1);
	c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
	c->values[MTY_CVALUE_THUMB_RY].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_RY].max = INT16_MAX;

	c->values[MTY_CVALUE_TRIGGER_L].data = d[4];
	c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
	c->values[MTY_CVALUE_TRIGGER_L].min = 0;
	c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

	c->values[MTY_CVALUE_TRIGGER_R].data = d[5];
	c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
	c->values[MTY_CVALUE_TRIGGER_R].min = 0;
	c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;

	c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = c->values[MTY_CVALUE_TRIGGER_L].data > 0;
	c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = c->values[MTY_CVALUE_TRIGGER_R].data > 0;

	bool up = d[2] & 0x01;
	bool down = d[2] & 0x02;
	bool left = d[2] & 0x04;
	bool right = d[2] & 0x08;

	c->values[MTY_CVALUE_DPAD].data = (up && right) ? 1 : (right && down) ? 3 :
		(down && left) ? 5 : (left && up) ? 7 : up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;
	c->values[MTY_CVALUE_DPAD].usage = 0x39;
	c->values[MTY_CVALUE_DPAD].min = 0;
	c->values[MTY_CVALUE_DPAD].max = 7;
}
