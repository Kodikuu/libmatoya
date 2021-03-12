// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include "hid/hid.h"

#include "hidpi.h"
#include <xinput.h>

typedef struct {
	WORD vid;
	WORD pid;
	uint8_t _pad[64];
} XINPUT_BASE_BUS_INFORMATION;

struct xip {
	bool disabled;
	bool was_enabled;
	DWORD packet;
	XINPUT_BASE_BUS_INFORMATION bbi;
};

struct hid {
	uint32_t id;
	MTY_Hash *devices;
	MTY_Hash *devices_rev;
	HID_CONNECT connect;
	HID_DISCONNECT disconnect;
	HID_REPORT report;
	void *opaque;

	// XInput
	struct xip xinput[4];
	HMODULE xinput1_4;
	DWORD (WINAPI *XInputGetState)(DWORD dwUserIndex, XINPUT_STATE *pState);
	DWORD (WINAPI *XInputSetState)(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration);
	DWORD (WINAPI *XInputGetBaseBusInformation)(DWORD dwUserIndex, XINPUT_BASE_BUS_INFORMATION *pBusInformation);
};

struct hdevice {
	RID_DEVICE_INFO di;
	PHIDP_PREPARSED_DATA ppd;
	HIDP_CAPS caps;
	HIDP_BUTTON_CAPS *bcaps;
	HIDP_VALUE_CAPS *vcaps;
	wchar_t *name;
	bool is_xinput;
	void *state;
	uint32_t id;
};


// HID

static void hid_device_destroy(void *opaque)
{
	if (!opaque)
		return;

	struct hdevice *ctx = opaque;

	MTY_Free(ctx->bcaps);
	MTY_Free(ctx->vcaps);
	MTY_Free(ctx->ppd);
	MTY_Free(ctx->name);
	MTY_Free(ctx->state);
	MTY_Free(ctx);
}

static struct hdevice *hid_device_create(HANDLE device)
{
	struct hdevice *ctx = MTY_Alloc(1, sizeof(struct hdevice));

	bool r = true;

	UINT size = 0;
	UINT e = GetRawInputDeviceInfo(device, RIDI_PREPARSEDDATA, NULL, &size);
	if (e != 0) {
		r = false;
		MTY_Log("'GetRawInputDeviceInfo' failed with error 0x%X", GetLastError());
		goto except;
	}

	ctx->ppd = MTY_Alloc(size, 1);
	e = GetRawInputDeviceInfo(device, RIDI_PREPARSEDDATA, ctx->ppd, &size);
	if (e == 0xFFFFFFFF || e == 0) {
		r = false;
		MTY_Log("'GetRawInputDeviceInfo' failed with error 0x%X", GetLastError());
		goto except;
	}

	NTSTATUS ne = HidP_GetCaps(ctx->ppd, &ctx->caps);
	if (ne != HIDP_STATUS_SUCCESS) {
		r = false;
		MTY_Log("'HidP_GetCaps' failed with NTSTATUS 0x%X", ne);
		goto except;
	}

	ctx->bcaps = MTY_Alloc(ctx->caps.NumberInputButtonCaps, sizeof(HIDP_BUTTON_CAPS));
	ne = HidP_GetButtonCaps(HidP_Input, ctx->bcaps, &ctx->caps.NumberInputButtonCaps, ctx->ppd);
	if (ne != HIDP_STATUS_SUCCESS) {
		r = false;
		MTY_Log("'HidP_GetButtonCaps' failed with NTSTATUS 0x%X", ne);
		goto except;
	}

	ctx->vcaps = MTY_Alloc(ctx->caps.NumberInputValueCaps, sizeof(HIDP_VALUE_CAPS));
	ne = HidP_GetValueCaps(HidP_Input, ctx->vcaps, &ctx->caps.NumberInputValueCaps, ctx->ppd);
	if (ne != HIDP_STATUS_SUCCESS) {
		r = false;
		MTY_Log("'HidP_GetValueCaps' failed with NTSTATUS 0x%X", ne);
		goto except;
	}

	ctx->di.cbSize = size = sizeof(RID_DEVICE_INFO);
	e = GetRawInputDeviceInfo(device, RIDI_DEVICEINFO, &ctx->di, &size);
	if (e == 0xFFFFFFFF || e == 0) {
		r = false;
		MTY_Log("'GetRawInputDeviceInfo' failed with error 0x%X", GetLastError());
		goto except;
	}

	size = 0;
	e = GetRawInputDeviceInfo(device, RIDI_DEVICENAME, NULL, &size);
	if (e != 0) {
		r = false;
		MTY_Log("'GetRawInputDeviceInfo' failed with error 0x%X", GetLastError());
		goto except;
	}

	ctx->name = MTY_Alloc(size, sizeof(wchar_t));
	e = GetRawInputDeviceInfo(device, RIDI_DEVICENAME, ctx->name, &size);
	if (e == 0xFFFFFFFF) {
		r = false;
		MTY_Log("'GetRawInputDeviceInfo' failed with error 0x%X", GetLastError());
		goto except;
	}

	ctx->is_xinput = wcsstr(ctx->name, L"IG_") ? true : false;
	ctx->state = MTY_Alloc(HID_STATE_MAX, 1);

	except:

	if (!r) {
		hid_device_destroy(ctx);
		ctx = NULL;
	}

	return ctx;
}

struct hid *mty_hid_create(HID_CONNECT connect, HID_DISCONNECT disconnect, HID_REPORT report, void *opaque)
{
	struct hid *ctx = MTY_Alloc(1, sizeof(struct hid));
	ctx->devices = MTY_HashCreate(0);
	ctx->devices_rev = MTY_HashCreate(0);
	ctx->connect = connect;
	ctx->disconnect = disconnect;
	ctx->report = report;
	ctx->opaque = opaque;
	ctx->id = 32;

	// XInput
	bool fallback = false;

	ctx->XInputGetState = XInputGetState;
	ctx->XInputSetState = XInputSetState;

	ctx->xinput1_4 = LoadLibrary(L"xinput1_4.dll");
	if (ctx->xinput1_4) {
		ctx->XInputGetState = (void *) GetProcAddress(ctx->xinput1_4, (LPCSTR) 100);
		if (!ctx->XInputGetState) {
			fallback = true;
			ctx->XInputGetState = XInputGetState;
		}

		ctx->XInputSetState = (void *) GetProcAddress(ctx->xinput1_4, "XInputSetState");
		if (!ctx->XInputSetState) {
			fallback = true;
			ctx->XInputSetState = XInputSetState;
		}

		ctx->XInputGetBaseBusInformation = (void *) GetProcAddress(ctx->xinput1_4, (LPCSTR) 104);
	}

	if (!ctx->xinput1_4 || !ctx->XInputGetBaseBusInformation || fallback)
		MTY_Log("Failed to load full XInput 1.4 interface");

	return ctx;
}

struct hdevice *mty_hid_get_device_by_id(struct hid *ctx, uint32_t id)
{
	return MTY_HashGetInt(ctx->devices_rev, id);
}

void mty_hid_destroy(struct hid **hid)
{
	if (!hid || !*hid)
		return;

	struct hid *ctx = *hid;

	if (ctx->xinput1_4)
		FreeLibrary(ctx->xinput1_4);

	MTY_HashDestroy(&ctx->devices, hid_device_destroy);
	MTY_HashDestroy(&ctx->devices_rev, NULL);

	MTY_Free(ctx);
	*hid = NULL;
}

void mty_hid_device_write(struct hdevice *ctx, const void *buf, size_t size)
{
	OVERLAPPED ov = {0};
	ov.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	HANDLE device = CreateFile(ctx->name, GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED, NULL);

	if (!device) {
		MTY_Log("'CreateFile' failed with error 0x%X", GetLastError());
		goto except;
	}

	if (!WriteFile(device, buf, (DWORD) size, NULL, &ov)) {
		DWORD e = GetLastError();
		if (e != ERROR_IO_PENDING) {
			MTY_Log("'WriteFile' failed with error 0x%X", e);
			goto except;
		}
	}

	if (WaitForSingleObject(ov.hEvent, 1000) == WAIT_OBJECT_0) {
		DWORD written = 0;
		if (!GetOverlappedResult(device, &ov, &written, FALSE)) {
			MTY_Log("'GetOverlappedResult' failed with error 0x%X", GetLastError());
			goto except;
		}
	}

	except:

	if (ov.hEvent)
		CloseHandle(ov.hEvent);

	if (device)
		CloseHandle(device);
}

bool mty_hid_device_feature(struct hdevice *ctx, void *buf, size_t size, size_t *size_out)
{
	bool r = true;

	*size_out = 0;
	OVERLAPPED ov = {0};

	HANDLE device = CreateFile(ctx->name, GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED, NULL);

	if (!device) {
		r = false;
		MTY_Log("'CreateFile' failed with error 0x%X", GetLastError());
		goto except;
	}

	DWORD out = 0;
	if (!DeviceIoControl(device, IOCTL_HID_GET_FEATURE, buf, (DWORD) size, buf, (DWORD) size, &out, &ov)) {
		DWORD e = GetLastError();
		if (e != ERROR_IO_PENDING) {
			r = false;
			MTY_Log("'DeviceIoControl' failed with error 0x%X", e);
			goto except;
		}
	}

	if (!GetOverlappedResult(device, &ov, &out, TRUE)) {
		r = false;
		MTY_Log("'GetOverlappedResult' failed with error 0x%X", GetLastError());
		goto except;
	}

	*size_out = out + 1;

	except:

	if (device)
		CloseHandle(device);

	return r;
}

void *mty_hid_device_get_state(struct hdevice *ctx)
{
	return ctx->state;
}

uint16_t mty_hid_device_get_vid(struct hdevice *ctx)
{
	return (uint16_t) ctx->di.hid.dwVendorId;
}

uint16_t mty_hid_device_get_pid(struct hdevice *ctx)
{
	return (uint16_t) ctx->di.hid.dwProductId;
}

uint32_t mty_hid_device_get_id(struct hdevice *ctx)
{
	return ctx->id;
}

uint32_t mty_hid_device_get_input_report_size(struct hdevice *ctx)
{
	return ctx->caps.InputReportByteLength;
}

bool mty_hid_device_force_default(struct hdevice *ctx)
{
	return false;
}

void mty_hid_default_state(struct hdevice *ctx, const void *buf, size_t size, MTY_Event *evt)
{
	MTY_ControllerEvent *c = &evt->controller;

	// Note: Some of these functions use PCHAR for a const buffer, thus the cast

	// Buttons
	for (uint32_t x = 0; x < ctx->caps.NumberInputButtonCaps; x++) {
		const HIDP_BUTTON_CAPS *bcap = &ctx->bcaps[x];

		// Should we consider usages other than 0x09 (Button)?
		if (bcap->UsagePage == 0x09) {
			c->numButtons += (uint8_t) (bcap->Range.UsageMax - bcap->Range.UsageMin + 1);

			ULONG n = MTY_CBUTTON_MAX;
			USAGE usages[MTY_CBUTTON_MAX];
			NTSTATUS e = HidP_GetUsages(HidP_Input, bcap->UsagePage, 0, usages, &n,
				ctx->ppd, (PCHAR) buf, (ULONG) size);

			if (e != HIDP_STATUS_SUCCESS && e != HIDP_STATUS_INCOMPATIBLE_REPORT_ID) {
				MTY_Log("'HidP_GetUsages' failed with NTSTATUS 0x%X", e);
				return;
			}

			if (e == HIDP_STATUS_SUCCESS) {
				for (ULONG y = 0; y < n; y++) {
					uint32_t i = usages[y] - bcap->Range.UsageMin;
					if (i < MTY_CBUTTON_MAX)
						c->buttons[i] = true;
				}
			}
		}
	}

	// Values
	for (uint32_t x = 0; x < ctx->caps.NumberInputValueCaps && c->numValues < MTY_CVALUE_MAX; x++) {
		const HIDP_VALUE_CAPS *vcap = &ctx->vcaps[x];

		ULONG value = 0;
		NTSTATUS e = HidP_GetUsageValue(HidP_Input, vcap->UsagePage, 0, vcap->Range.UsageMin,
			&value, ctx->ppd, (PCHAR) buf, (ULONG) size);

		if (e != HIDP_STATUS_SUCCESS && e != HIDP_STATUS_INCOMPATIBLE_REPORT_ID) {
			MTY_Log("'HidP_GetUsageValue' failed with NTSTATUS 0x%X", e);
			return;
		}

		if (e == HIDP_STATUS_SUCCESS) {
			MTY_Value *v = &c->values[c->numValues++];

			v->usage = vcap->Range.UsageMin;
			v->data = (int16_t) value;
			v->min = (int16_t) vcap->LogicalMin;
			v->max = (int16_t) vcap->LogicalMax;
		}
	}

	evt->type = MTY_EVENT_CONTROLLER;
	c->vid = (uint16_t) ctx->di.hid.dwVendorId;
	c->pid = (uint16_t) ctx->di.hid.dwProductId;
	c->type = MTY_CTYPE_DEFAULT;
	c->id = ctx->id;
}

void mty_hid_default_rumble(struct hid *ctx, uint32_t id, uint16_t low, uint16_t high)
{
}


// Win32 RAWINPUT interop

void mty_hid_win32_report(struct hid *ctx, intptr_t device, const void *buf, size_t size)
{
	struct hdevice *dev = MTY_HashGetInt(ctx->devices, device);

	if (dev && !dev->is_xinput)
		ctx->report(dev, buf, size, ctx->opaque);
}

void mty_hid_win32_device_change(struct hid *ctx, intptr_t wparam, intptr_t lparam)
{
	if (wparam == GIDC_ARRIVAL) {
		for (uint8_t x = 0; x < 4; x++)
			ctx->xinput[x].disabled = false;

		struct hdevice *dev = hid_device_create((HANDLE) lparam);
		if (dev) {
			dev->id = ctx->id++;
			mty_hid_destroy(MTY_HashSetInt(ctx->devices, lparam, dev));
			MTY_HashSetInt(ctx->devices_rev, dev->id, dev);

			if (!dev->is_xinput)
				ctx->connect(dev, ctx->opaque);
		}

	} else if (wparam == GIDC_REMOVAL) {
		struct hdevice *dev = MTY_HashPopInt(ctx->devices, lparam);
		if (dev) {
			if (!dev->is_xinput)
				ctx->disconnect(dev, ctx->opaque);

			MTY_HashPopInt(ctx->devices_rev, dev->id);
			hid_device_destroy(dev);
		}
	}
}

void mty_hid_win32_listen(void *hwnd)
{
	RAWINPUTDEVICE rid[3] = {
		{0x01, 0x04, RIDEV_DEVNOTIFY | RIDEV_INPUTSINK, (HWND) hwnd},
		{0x01, 0x05, RIDEV_DEVNOTIFY | RIDEV_INPUTSINK, (HWND) hwnd},
		{0x01, 0x08, RIDEV_DEVNOTIFY | RIDEV_INPUTSINK, (HWND) hwnd},
	};

	if (!RegisterRawInputDevices(rid, 3, sizeof(RAWINPUTDEVICE)))
		MTY_Log("'RegisterRawInputDevices' failed with error 0x%X", GetLastError());
}


// XInput interop

static void hid_xinput_to_mty(const XINPUT_STATE *xstate, MTY_Event *evt)
{
	WORD b = xstate->Gamepad.wButtons;
	MTY_ControllerEvent *c = &evt->controller;

	c->buttons[MTY_CBUTTON_X] = b & XINPUT_GAMEPAD_X;
	c->buttons[MTY_CBUTTON_A] = b & XINPUT_GAMEPAD_A;
	c->buttons[MTY_CBUTTON_B] = b & XINPUT_GAMEPAD_B;
	c->buttons[MTY_CBUTTON_Y] = b & XINPUT_GAMEPAD_Y;
	c->buttons[MTY_CBUTTON_LEFT_SHOULDER] = b & XINPUT_GAMEPAD_LEFT_SHOULDER;
	c->buttons[MTY_CBUTTON_RIGHT_SHOULDER] = b & XINPUT_GAMEPAD_RIGHT_SHOULDER;
	c->buttons[MTY_CBUTTON_LEFT_TRIGGER] = xstate->Gamepad.bLeftTrigger > 0;
	c->buttons[MTY_CBUTTON_RIGHT_TRIGGER] = xstate->Gamepad.bRightTrigger > 0;
	c->buttons[MTY_CBUTTON_BACK] = b & XINPUT_GAMEPAD_BACK;
	c->buttons[MTY_CBUTTON_START] = b & XINPUT_GAMEPAD_START;
	c->buttons[MTY_CBUTTON_LEFT_THUMB] = b & XINPUT_GAMEPAD_LEFT_THUMB;
	c->buttons[MTY_CBUTTON_RIGHT_THUMB] = b & XINPUT_GAMEPAD_RIGHT_THUMB;
	c->buttons[MTY_CBUTTON_GUIDE] = b & 0x0400;

	c->values[MTY_CVALUE_THUMB_LX].data = xstate->Gamepad.sThumbLX;
	c->values[MTY_CVALUE_THUMB_LX].usage = 0x30;
	c->values[MTY_CVALUE_THUMB_LX].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_LX].max = INT16_MAX;

	c->values[MTY_CVALUE_THUMB_LY].data = xstate->Gamepad.sThumbLY;
	c->values[MTY_CVALUE_THUMB_LY].usage = 0x31;
	c->values[MTY_CVALUE_THUMB_LY].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_LY].max = INT16_MAX;

	c->values[MTY_CVALUE_THUMB_RX].data = xstate->Gamepad.sThumbRX;
	c->values[MTY_CVALUE_THUMB_RX].usage = 0x32;
	c->values[MTY_CVALUE_THUMB_RX].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_RX].max = INT16_MAX;

	c->values[MTY_CVALUE_THUMB_RY].data = xstate->Gamepad.sThumbRY;
	c->values[MTY_CVALUE_THUMB_RY].usage = 0x35;
	c->values[MTY_CVALUE_THUMB_RY].min = INT16_MIN;
	c->values[MTY_CVALUE_THUMB_RY].max = INT16_MAX;

	c->values[MTY_CVALUE_TRIGGER_L].data = xstate->Gamepad.bLeftTrigger;
	c->values[MTY_CVALUE_TRIGGER_L].usage = 0x33;
	c->values[MTY_CVALUE_TRIGGER_L].min = 0;
	c->values[MTY_CVALUE_TRIGGER_L].max = UINT8_MAX;

	c->values[MTY_CVALUE_TRIGGER_R].data = xstate->Gamepad.bRightTrigger;
	c->values[MTY_CVALUE_TRIGGER_R].usage = 0x34;
	c->values[MTY_CVALUE_TRIGGER_R].min = 0;
	c->values[MTY_CVALUE_TRIGGER_R].max = UINT8_MAX;

	bool up = b & XINPUT_GAMEPAD_DPAD_UP;
	bool down = b & XINPUT_GAMEPAD_DPAD_DOWN;
	bool left = b & XINPUT_GAMEPAD_DPAD_LEFT;
	bool right = b & XINPUT_GAMEPAD_DPAD_RIGHT;

	c->values[MTY_CVALUE_DPAD].data = (up && right) ? 1 : (right && down) ? 3 :
		(down && left) ? 5 : (left && up) ? 7 : up ? 0 : right ? 2 : down ? 4 : left ? 6 : 8;
	c->values[MTY_CVALUE_DPAD].usage = 0x39;
	c->values[MTY_CVALUE_DPAD].min = 0;
	c->values[MTY_CVALUE_DPAD].max = 7;
}

void mty_hid_xinput_rumble(struct hid *ctx, uint32_t id, uint16_t low, uint16_t high)
{
	if (id >= 4)
		return;

	if (!ctx->xinput[id].disabled) {
		XINPUT_VIBRATION vb = {0};
		vb.wLeftMotorSpeed = low;
		vb.wRightMotorSpeed = high;

		DWORD e = ctx->XInputSetState(id, &vb);
		if (e != ERROR_SUCCESS)
			MTY_Log("'XInputSetState' failed with error 0x%X", e);
	}
}

void mty_hid_xinput_state(struct hid *ctx, MTY_EventFunc func, void *opaque)
{
	for (uint8_t x = 0; x < 4; x++) {
		struct xip *xip = &ctx->xinput[x];

		if (!xip->disabled) {
			MTY_Event evt = {0};
			evt.controller.type = MTY_CTYPE_XINPUT;
			evt.controller.numButtons = 13;
			evt.controller.numValues = 7;
			evt.controller.id = x;

			XINPUT_STATE xstate;
			if (ctx->XInputGetState(x, &xstate) == ERROR_SUCCESS) {
				if (!xip->was_enabled) {
					xip->bbi.vid = 0x045E;
					xip->bbi.pid = 0x0000;

					if (ctx->XInputGetBaseBusInformation) {
						DWORD e = ctx->XInputGetBaseBusInformation(x, &xip->bbi);
						if (e != ERROR_SUCCESS)
							MTY_Log("'XInputGetBaseBusInformation' failed with error 0x%X", e);
					}

					xip->was_enabled = true;
				}

				if (xstate.dwPacketNumber != xip->packet) {
					evt.type = MTY_EVENT_CONTROLLER;
					evt.controller.vid = xip->bbi.vid;
					evt.controller.pid = xip->bbi.pid;

					hid_xinput_to_mty(&xstate, &evt);
					func(&evt, opaque);

					xip->packet = xstate.dwPacketNumber;
				}
			} else {
				xip->disabled = true;

				if (xip->was_enabled) {
					evt.type = MTY_EVENT_DISCONNECT;
					func(&evt, opaque);

					xip->was_enabled = false;
				}
			}
		}
	}
}
