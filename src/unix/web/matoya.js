// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Private helpers

let MODULE;

function mem() {
	return MODULE.instance.exports.memory.buffer;
}

function mem_view() {
	return new DataView(mem());
}

function char_to_js(buf) {
	let str = '';

	for (let x = 0; x < 0x7FFFFFFF && x < buf.length; x++) {
		if (buf[x] == 0)
			break;

		str += String.fromCharCode(buf[x]);
	}

	return str;
}

function b64_to_buf(str) {
	return Uint8Array.from(atob(str), c => c.charCodeAt(0))
}

function buf_to_b64(buf) {
	let str = '';
	for (let x = 0; x < buf.length; x++)
		str += String.fromCharCode(buf[x]);

	return btoa(str);
}


// Utility

let MTY_ALLOC;
let MTY_FREE;

function MTY_CFunc(ptr) {
	return MODULE.instance.exports.__indirect_function_table.get(ptr);
}

function MTY_Alloc(size, el) {
	return MTY_CFunc(MTY_ALLOC)(size, el ? el : 1);
}

function MTY_Free(ptr) {
	MTY_CFunc(MTY_FREE)(ptr);
}

function MTY_SetUint32(ptr, value) {
	mem_view().setUint32(ptr, value, true);
}

function MTY_SetUint16(ptr, value) {
	mem_view().setUint16(ptr, value, true);
}

function MTY_SetInt32(ptr, value) {
	mem_view().setInt32(ptr, value, true);
}

function MTY_SetInt8(ptr, value) {
	mem_view().setInt8(ptr, value);
}

function MTY_SetFloat(ptr, value) {
	mem_view().setFloat32(ptr, value, true);
}

function MTY_SetUint64(ptr, value) {
	mem_view().setBigUint64(ptr, BigInt(value), true);
}

function MTY_GetUint32(ptr) {
	return mem_view().getUint32(ptr, true);
}

function MTY_Memcpy(cptr, abuffer) {
	const heap = new Uint8Array(mem(), cptr, abuffer.length);
	heap.set(abuffer);
}

function MTY_StrToJS(ptr) {
	return char_to_js(new Uint8Array(mem(), ptr));
}

function MTY_StrToC(js_str, ptr) {
	const view = new Uint8Array(mem(), ptr);

	// FIXME No bounds checking!
	let len = 0;
	for (; len < js_str.length; len++)
		view[len] = js_str.charCodeAt(len);

	// '\0' character
	view[len] = 0;

	return ptr;
}


// GL

let GL;
let GL_OBJ_INDEX = 0;
let GL_OBJ = {};

function gl_new(obj) {
	GL_OBJ[GL_OBJ_INDEX] = obj;

	return GL_OBJ_INDEX++;
}

function gl_del(index) {
	let obj = GL_OBJ[index];

	GL_OBJ[index] = undefined;
	delete GL_OBJ[index];

	return obj;
}

function gl_obj(index) {
	return GL_OBJ[index];
}

const GL_API = {
	glGenFramebuffers: function (n, ids) {
		for (let x = 0; x < n; x++)
			MTY_SetUint32(ids + x * 4, gl_new(GL.createFramebuffer()));
	},
	glDeleteFramebuffers: function (n, ids) {
		for (let x = 0; x < n; x++)
			GL.deleteFramebuffer(gl_del(MTY_GetUint32(ids + x * 4)));
	},
	glBindFramebuffer: function (target, fb) {
		GL.bindFramebuffer(target, fb ? gl_obj(fb) : null);
	},
	glBlitFramebuffer: function (srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter) {
		GL.blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
	},
	glFramebufferTexture2D: function (target, attachment, textarget, texture, level) {
		GL.framebufferTexture2D(target, attachment, textarget, gl_obj(texture), level);
	},
	glEnable: function (cap) {
		GL.enable(cap);
	},
	glIsEnabled: function (cap) {
		return GL.isEnabled(cap);
	},
	glDisable: function (cap) {
		GL.disable(cap);
	},
	glViewport: function (x, y, width, height) {
		GL.viewport(x, y, width, height);
	},
	glGetIntegerv: function (name, data) {
		const p = GL.getParameter(name);

		switch (name) {
			// object
			case GL.READ_FRAMEBUFFER_BINDING:
			case GL.DRAW_FRAMEBUFFER_BINDING:
			case GL.ARRAY_BUFFER_BINDING:
			case GL.TEXTURE_BINDING_2D:
			case GL.CURRENT_PROGRAM:
				MTY_SetUint32(data, gl_new(p));
				break;

			// int32[4]
			case GL.VIEWPORT:
			case GL.SCISSOR_BOX:
				for (let x = 0; x < 4; x++)
					MTY_SetUint32(data + x * 4, p[x]);
				break;

			// int
			case GL.ACTIVE_TEXTURE:
			case GL.BLEND_SRC_RGB:
			case GL.BLEND_DST_RGB:
			case GL.BLEND_SRC_ALPHA:
			case GL.BLEND_DST_ALPHA:
			case GL.BLEND_EQUATION_RGB:
			case GL.BLEND_EQUATION_ALPHA:
				MTY_SetUint32(data, p);
				break;
		}

		MTY_SetUint32(data, p);
	},
	glGetFloatv: function (name, data) {
		switch (name) {
			case GL.COLOR_CLEAR_VALUE:
				const p = GL.getParameter(name);

				for (let x = 0; x < 4; x++)
					MTY_SetFloat(data + x * 4, p[x]);
				break;
		}
	},
	glBindTexture: function (target, texture) {
		GL.bindTexture(target, texture ? gl_obj(texture) : null);
	},
	glDeleteTextures: function (n, ids) {
		for (let x = 0; x < n; x++)
			GL.deleteTexture(gl_del(MTY_GetUint32(ids + x * 4)));
	},
	glTexParameteri: function (target, pname, param) {
		GL.texParameteri(target, pname, param);
	},
	glGenTextures: function (n, ids) {
		for (let x = 0; x < n; x++)
			MTY_SetUint32(ids + x * 4, gl_new(GL.createTexture()));
	},
	glTexImage2D: function (target, level, internalformat, width, height, border, format, type, data) {
		GL.texImage2D(target, level, internalformat, width, height, border, format, type,
			new Uint8Array(mem(), data));
	},
	glTexSubImage2D: function (target, level, xoffset, yoffset, width, height, format, type, pixels) {
		GL.texSubImage2D(target, level, xoffset, yoffset, width, height, format, type,
			new Uint8Array(mem(), pixels));
	},
	glDrawElements: function (mode, count, type, indices) {
		GL.drawElements(mode, count, type, indices);
	},
	glGetAttribLocation: function (program, c_name) {
		return GL.getAttribLocation(gl_obj(program), MTY_StrToJS(c_name));
	},
	glShaderSource: function (shader, count, c_strings, c_len) {
		let source = '';
		for (let x = 0; x < count; x++)
			source += MTY_StrToJS(MTY_GetUint32(c_strings + x * 4));

		GL.shaderSource(gl_obj(shader), source);
	},
	glBindBuffer: function (target, buffer) {
		GL.bindBuffer(target, buffer ? gl_obj(buffer) : null);
	},
	glVertexAttribPointer: function (index, size, type, normalized, stride, pointer) {
		GL.vertexAttribPointer(index, size, type, normalized, stride, pointer);
	},
	glCreateProgram: function () {
		return gl_new(GL.createProgram());
	},
	glUniform1i: function (loc, v0) {
		GL.uniform1i(gl_obj(loc), v0);
	},
	glUniform1f: function (loc, v0) {
		GL.uniform1f(gl_obj(loc), v0);
	},
	glUniform4i: function (loc, v0, v1, v2, v3) {
		GL.uniform4i(gl_obj(loc), v0, v1, v2, v3);
	},
	glUniform4f: function (loc, v0, v1, v2, v3) {
		GL.uniform4f(gl_obj(loc), v0, v1, v2, v3);
	},
	glActiveTexture: function (texture) {
		GL.activeTexture(texture);
	},
	glDeleteBuffers: function (n, ids) {
		for (let x = 0; x < n; x++)
			GL.deleteBuffer(gl_del(MTY_GetUint32(ids + x * 4)));
	},
	glEnableVertexAttribArray: function (index) {
		GL.enableVertexAttribArray(index);
	},
	glBufferData: function (target, size, data, usage) {
		GL.bufferData(target, new Uint8Array(mem(), data, size), usage);
	},
	glDeleteShader: function (shader) {
		GL.deleteShader(gl_del(shader));
	},
	glGenBuffers: function (n, ids) {
		for (let x = 0; x < n; x++)
			MTY_SetUint32(ids + x * 4, gl_new(GL.createBuffer()));
	},
	glCompileShader: function (shader) {
		GL.compileShader(gl_obj(shader));
	},
	glLinkProgram: function (program) {
		GL.linkProgram(gl_obj(program));
	},
	glGetUniformLocation: function (program, name) {
		return gl_new(GL.getUniformLocation(gl_obj(program), MTY_StrToJS(name)));
	},
	glCreateShader: function (type) {
		return gl_new(GL.createShader(type));
	},
	glAttachShader: function (program, shader) {
		GL.attachShader(gl_obj(program), gl_obj(shader));
	},
	glUseProgram: function (program) {
		GL.useProgram(program ? gl_obj(program) : null);
	},
	glGetShaderiv: function (shader, pname, params) {
		if (pname == 0x8B81) {
			let ok = GL.getShaderParameter(gl_obj(shader), GL.COMPILE_STATUS);
			MTY_SetUint32(params, ok);

			if (!ok)
				console.warn(GL.getShaderInfoLog(gl_obj(shader)));

		} else {
			MTY_SetUint32(params, 0);
		}
	},
	glDetachShader: function (program, shader) {
		GL.detachShader(gl_obj(program), gl_obj(shader));
	},
	glDeleteProgram: function (program) {
		GL.deleteProgram(gl_del(program));
	},
	glClear: function (mask) {
		GL.clear(mask);
	},
	glClearColor: function (red, green, blue, alpha) {
		GL.clearColor(red, green, blue, alpha);
	},
	glGetError: function () {
		return GL.getError();
	},
	glGetShaderInfoLog: function () {
		// FIXME Logged automatically as part of glGetShaderiv
	},
	glFinish: function () {
		GL.finish();
	},
	glScissor: function (x, y, width, height) {
		GL.scissor(x, y, width, height);
	},
	glBlendFunc: function (sfactor, dfactor) {
		GL.blendFunc(sfactor, dfactor);
	},
	glBlendEquation: function (mode) {
		GL.blendEquation(mode);
	},
	glUniformMatrix4fv: function (loc, count, transpose, value) {
		GL.uniformMatrix4fv(gl_obj(loc), transpose, new Float32Array(mem(), value, 4 * 4 * count));
	},
	glBlendEquationSeparate: function (modeRGB, modeAlpha) {
		GL.blendEquationSeparate(modeRGB, modeAlpha);
	},
	glBlendFuncSeparate: function (srcRGB, dstRGB, srcAlpha, dstAlpha) {
		GL.blendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
	},
	glGetProgramiv: function (program, pname, params) {
		MTY_SetUint32(params, GL.getProgramParameter(gl_obj(program), pname));
	},
};


// Matoya audio API
let AC = {};

const MTY_AUDIO_API = {
	MTY_AudioCreate: function (sampleRate) {
		AC.sample_rate = sampleRate;
		AC.playing = false;

		return 1; // In case the app checks for NULL
	},
	MTY_AudioDestroy: function (_) {
		MTY_SetUint32(_, 0);
		AC = {};
	},
	MTY_AudioPlay: function () {
		AC.playing = true;
	},
	MTY_AudioStop: function () {
		AC.playing = false;
	},
	MTY_AudioQueue: function (_, frames, count) {
		if (!AC.ctx) {
			AC.ctx = new AudioContext();
			AC.next_time = AC.ctx.currentTime;
		}

		if (AC.next_time - AC.ctx.currentTime > 0.200)
			return;

		if (AC.next_time < AC.ctx.currentTime)
			AC.next_time = AC.ctx.currentTime + 0.070;

		const buf = AC.ctx.createBuffer(2, count, AC.sample_rate);
		const left = buf.getChannelData(0);
		const right = buf.getChannelData(1);

		const src = new Int16Array(mem(), frames);

		let offset = 0;
		for (let x = 0; x < count * 2; x += 2) {
			left[offset] = src[x] / 32768;
			right[offset] = src[x + 1] / 32768;
			offset++;
		}

		const buf_source = AC.ctx.createBufferSource();
		buf_source.buffer = buf;
		buf_source.connect(AC.ctx.destination);
		buf_source.start(AC.next_time);
		AC.next_time += buf.duration;
	},
	MTY_AudioIsPlaying: function () {
		return AC.playing;
	},
	MTY_AudioGetQueuedMs: function () {
		if (AC.ctx) {
			const queued_ms = Math.round((AC.next_time - AC.ctx.currentTime) * 1000.0);
		}
		return 0;
	},
};


// Matoya web API
const _MTY = {
	cbuf: null,
	kbMap: null,
	keysRev: {},
	wakeLock: null,
	reqs: {},
	reqIndex: 0,
	endFunc: () => {},
	cursorId: 0,
	cursorCache: {},
	cursorClass: '',
	defaultCursor: false,
	synthesizeEsc: true,
	relative: false,
	action: null,
};

let CLIPBOARD;
let KEYS = {};
let GPS = [false, false, false, false];

const MTY_ASYNC_OK = 0;
const MTY_ASYNC_DONE = 1;
const MTY_ASYNC_CONTINUE = 2;
const MTY_ASYNC_ERROR = 3;

function get_mods(ev) {
	let mods = 0;

	if (ev.shiftKey) mods |= 0x01;
	if (ev.ctrlKey)  mods |= 0x02;
	if (ev.altKey)   mods |= 0x04;
	if (ev.metaKey)  mods |= 0x08;

	if (ev.getModifierState("CapsLock")) mods |= 0x10;
	if (ev.getModifierState("NumLock") ) mods |= 0x20;

	return mods;
}

function run_action() {
	setTimeout(() => {
		if (_MTY.action) {
			_MTY.action();
			_MTY.action = null;
		}
	}, 100);
}

function MTY_SetAction(action) {
	_MTY.action = action;

	// In case click handler doesn't happen
	run_action();
}

function scaled(num) {
	return Math.round(num * window.devicePixelRatio);
}

function correct_relative() {
	if (!document.pointerLockElement && _MTY.relative)
		GL.canvas.requestPointerLock();
}

function poll_gamepads(app, controller) {
	const gps = navigator.getGamepads();

	for (let x = 0; x < 4; x++) {
		const gp = gps[x];

		if (gp) {
			let state = 0;

			// Connected
			if (!GPS[x]) {
				GPS[x] = true;
				state = 1;
			}

			let lx = 0;
			let ly = 0;
			let rx = 0;
			let ry = 0;
			let lt = 0;
			let rt = 0;
			let buttons = 0;

			if (gp.buttons) {
				lt = gp.buttons[6].value;
				rt = gp.buttons[7].value;

				for (let i = 0; i < gp.buttons.length && i < 32; i++)
					if (gp.buttons[i].pressed)
						buttons |= 1 << i;
			}

			if (gp.axes) {
				if (gp.axes[0]) lx = gp.axes[0];
				if (gp.axes[1]) ly = gp.axes[1];
				if (gp.axes[2]) rx = gp.axes[2];
				if (gp.axes[3]) ry = gp.axes[3];
			}

			MTY_CFunc(controller)(app, x, state, buttons, lx, ly, rx, ry, lt, rt);

		// Disconnected
		} else if (GPS[x]) {
			MTY_CFunc(controller)(app, x, 2, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

			GPS[x] = false;
		}
	}
}

const MTY_WEB_API = {
	// net
	MTY_HttpAsyncCreate: function (num_threads) {
	},
	MTY_HttpAsyncDestroy: function () {
	},
	MTY_HttpParseUrl: function (url_c, host_c_out, host_size, path_c_out, path_size) {
		const url = MTY_StrToJS(url_c);

		try {
			const url_obj = new URL(url);
			const path = url_obj.pathname + url_obj.search;

			MTY_StrToC(url_obj.host, host_c_out);
			MTY_StrToC(path, path_c_out);

			return true;

		} catch (err) {
			console.error(err);
		}

		return false;
	},
	MTY_HttpEncodeUrl: function(src, dst, dst_len) {
		// No-op, automatically converted in fetch
		MTY_StrToC(MTY_StrToJS(src), dst);
	},
	MTY_HttpAsyncRequest: function(index, chost, secure, cmethod,
		cpath, cheaders, cbody, bodySize, timeout, func)
	{
		const req = ++_MTY.reqIndex;
		MTY_SetUint32(index, req);

		_MTY.reqs[req] = {
			async: MTY_ASYNC_CONTINUE,
			func: func,
		};

		const scheme = secure ? 'https' : 'http';
		const method = MTY_StrToJS(cmethod);
		const host = MTY_StrToJS(chost);
		const path = MTY_StrToJS(cpath);
		const headers_str = MTY_StrToJS(cheaders);
		const body = cbody ? MTY_StrToJS(cbody) : undefined;
		const url = scheme + '://' + host + path;

		const headers = {};
		const headers_nl = headers_str.split('\n');
		for (let x = 0; x < headers_nl.length; x++) {
			const pair = headers_nl[x];
			const pair_split = pair.split(':');

			if (pair_split[0] && pair_split[1])
				headers[pair_split[0]] = pair_split[1];
		}

		fetch(url, {
			method: method,
			headers: headers,
			body: body

		}).then((response) => {
			const data = _MTY.reqs[req];
			data.status = response.status;

			return response.arrayBuffer();

		}).then((body) => {
			const data = _MTY.reqs[req];
			data.response = new Uint8Array(body);
			data.async = MTY_ASYNC_OK;

		}).catch((err) => {
			const data = _MTY.reqs[req];
			console.error(err);
			data.status = 0;
			data.async = MTY_ASYNC_ERROR;
		});
	},
	MTY_HttpAsyncPoll: function(index, response, responseSize, code) {
		const data = _MTY.reqs[index];

		if (data == undefined || data.async == MTY_ASYNC_DONE)
			return MTY_ASYNC_DONE;

		if (data.async == MTY_ASYNC_CONTINUE)
			return MTY_ASYNC_CONTINUE;

		MTY_SetUint32(code, data.status);

		if (data.response != undefined) {
			MTY_SetUint32(responseSize, data.response.length);

			if (data.buf == undefined) {
				data.buf = MTY_Alloc(data.response.length + 1);
				MTY_Memcpy(data.buf, data.response);
			}

			MTY_SetUint32(response, data.buf);

			if (data.async == MTY_ASYNC_OK && data.func) {
				MTY_CFunc(data.func)(data.status, response, responseSize);
				data.buf = MTY_GetUint32(response);
			}
		}

		const r = data.async;
		data.async = MTY_ASYNC_DONE;

		return r;
	},
	MTY_HttpAsyncClear: function (index) {
		const req = MTY_GetUint32(index);
		const data = _MTY.reqs[req];

		if (data == undefined)
			return;

		MTY_Free(data.buf);
		delete _MTY.reqs[req];

		MTY_SetUint32(index, 0);
	},

	// crypto
	MTY_CryptoHash: function (algo, input, inputSize, key, keySize, output, outputSize) {
	},
	MTY_RandomBytes: function (cbuf, size) {
		const buf = new Uint8Array(mem(), size);
		Crypto.getRandomValues(buf);
	},

	// app-misc
	MTY_ProtocolHandler: function (uri, token) {
		MTY_SetAction(() => {
			window.open(MTY_StrToJS(uri), '_blank');
		});
	},

	// unistd
	gethostname: function (cbuf, size) {
		MTY_StrToC(location.hostname, cbuf);
	},
	flock: function (fd, flags) {
		return 0;
	},

	// browser
	web_set_fullscreen: function (fullscreen) {
		if (fullscreen && !document.fullscreenElement) {
			if (navigator.keyboard)
				navigator.keyboard.lock(["Escape"]);

			document.documentElement.requestFullscreen();

		} else if (!fullscreen && document.fullscreenElement) {
			document.exitFullscreen();

			if (navigator.keyboard)
				navigator.keyboard.unlock();
		}
	},
	web_get_fullscreen: function () {
		return document.fullscreenElement ? true : false;
	},
	web_set_mem_funcs: function (alloc, free) {
		MTY_ALLOC = alloc;
		MTY_FREE = free;

		// Global buffers for scratch heap space
		_MTY.cbuf = MTY_Alloc(1024);
	},
	web_set_key: function (reverse, code, key) {
		const str = MTY_StrToJS(code);
		KEYS[str] = key;

		if (reverse)
			_MTY.keysRev[key] = str;
	},
	web_get_key: function (key, cbuf, len) {
		const code = _MTY.keysRev[key];

		if (code != undefined) {
			if (_MTY.kbMap) {
				const text = _MTY.kbMap.get(code);
				if (text) {
					MTY_StrToC(text.toUpperCase(), cbuf);
					return true;
				}
			}

			MTY_StrToC(code, cbuf);
			return true;
		}

		return false;
	},
	web_wake_lock: async function (enable) {
		try {
			if (!enable && !_MTY.wakeLock) {
				_MTY.wakeLock = await navigator.wakeLock.request('screen');

			} else if (enable && _MTY.wakeLock) {
				_MTY.wakeLock.release();
				_MTY.wakeLock = undefined;
			}
		} catch (e) {
			_MTY.wakeLock = undefined;
		}
	},
	web_rumble_gamepad: function (id, low, high) {
		const gps = navigator.getGamepads();
		const gp = gps[id];

		if (gp && gp.vibrationActuator)
			gp.vibrationActuator.playEffect('dual-rumble', {
				startDelay: 0,
				duration: 2000,
				weakMagnitude: low,
				strongMagnitude: high,
			});
	},
	web_show_cursor: function (show) {
		GL.canvas.style.cursor = show ? '': 'none';
	},
	web_get_clipboard: function () {
		CLIPBOARD.focus();
		CLIPBOARD.select();
		document.execCommand('paste');

		const text_c = MTY_Alloc(CLIPBOARD.value.length * 4);
		MTY_StrToC(CLIPBOARD.value, text_c);

		return text_c;
	},
	web_set_clipboard: function (text_c) {
		CLIPBOARD.value = MTY_StrToJS(text_c);
		CLIPBOARD.focus();
		CLIPBOARD.select();
		document.execCommand('copy');
	},
	web_set_pointer_lock: function (enable) {
		if (enable && !document.pointerLockElement) {
			GL.canvas.requestPointerLock();

		} else if (!enable && document.pointerLockElement) {
			_MTY.synthesizeEsc = false;
			document.exitPointerLock();
		}

		_MTY.relative = enable;
	},
	web_get_relative: function () {
		return _MTY.relative;
	},
	web_has_focus: function () {
		return document.hasFocus();
	},
	web_is_visible: function () {
		if (document.hidden != undefined) {
			return !document.hidden;

		} else if (document.webkitHidden != undefined) {
			return !document.webkitHidden;
		}

		return true;
	},
	web_get_size: function (c_width, c_height) {
		MTY_SetUint32(c_width, GL.drawingBufferWidth);
		MTY_SetUint32(c_height, GL.drawingBufferHeight);
	},
	web_get_screen_size: function (c_width, c_height) {
		MTY_SetUint32(c_width, screen.width);
		MTY_SetUint32(c_height, screen.height);
	},
	web_set_title: function (title) {
		document.title = MTY_StrToJS(title);
	},
	web_use_default_cursor: function (use_default) {
		if (_MTY.cursorClass.length > 0) {
			if (use_default) {
				GL.canvas.classList.remove(_MTY.cursorClass);

			} else {
				GL.canvas.classList.add(_MTY.cursorClass);
			}
		}

		_MTY.defaultCursor = use_default;
	},
	web_set_png_cursor: function (buffer, size, hot_x, hot_y) {
		if (buffer) {
			const buf = new Uint8Array(mem(), buffer, size);
			const b64_png = buf_to_b64(buf);

			if (!_MTY.cursorCache[b64_png]) {
				_MTY.cursorCache[b64_png] = `cursor-x-${_MTY.cursorId}`;

				const style = document.createElement('style');
				style.type = 'text/css';
				style.innerHTML = `.cursor-x-${_MTY.cursorId++} ` +
					`{cursor: url(data:image/png;base64,${b64_png}) ${hot_x} ${hot_y}, auto;}`;
				document.querySelector('head').appendChild(style);
			}

			if (_MTY.cursorClass.length > 0)
				GL.canvas.classList.remove(_MTY.cursorClass);

			_MTY.cursorClass = _MTY.cursorCache[b64_png];

			if (!_MTY.defaultCursor)
				GL.canvas.classList.add(_MTY.cursorClass);

		} else {
			if (!_MTY.defaultCursor && _MTY.cursorClass.length > 0)
				GL.canvas.classList.remove(_MTY.cursorClass);

			_MTY.cursorClass = '';
		}
	},
	web_get_pixel_ratio: function () {
		return window.devicePixelRatio;
	},
	web_attach_events: function (app, mouse_motion, mouse_button, mouse_wheel, keyboard, focus, drop) {
		GL.canvas.addEventListener('mousemove', (ev) => {
			let x = scaled(ev.clientX);
			let y = scaled(ev.clientY);

			if (_MTY.relative) {
				x = ev.movementX;
				y = ev.movementY;
			}

			MTY_CFunc(mouse_motion)(app, _MTY.relative, x, y);
		});

		document.addEventListener('pointerlockchange', (ev) => {
			// Left relative via the ESC key, which swallows a natural ESC keypress
			if (!document.pointerLockElement && _MTY.synthesizeEsc) {
				MTY_CFunc(keyboard)(app, true, KEYS['Escape'], 0, 0);
				MTY_CFunc(keyboard)(app, false, KEYS['Escape'], 0, 0);
			}

			_MTY.synthesizeEsc = true;
		});

		window.addEventListener('click', (ev) => {
			// Popup blockers can interfere with window.open if not called from within the 'click' listener
			run_action();
			ev.preventDefault();
		});

		window.addEventListener('mousedown', (ev) => {
			correct_relative();
			ev.preventDefault();
			MTY_CFunc(mouse_button)(app, true, ev.button, scaled(ev.clientX), scaled(ev.clientY));
		});

		window.addEventListener('mouseup', (ev) => {
			ev.preventDefault();
			MTY_CFunc(mouse_button)(app, false, ev.button, scaled(ev.clientX), scaled(ev.clientY));
		});

		GL.canvas.addEventListener('contextmenu', (ev) => {
			ev.preventDefault();
		});

		GL.canvas.addEventListener('wheel', (ev) => {
			let x = ev.deltaX > 0 ? 120 : ev.deltaX < 0 ? -120 : 0;
			let y = ev.deltaY > 0 ? 120 : ev.deltaY < 0 ? -120 : 0;
			MTY_CFunc(mouse_wheel)(app, x, y);
		}, {passive: true});

		window.addEventListener('keydown', (ev) => {
			correct_relative();
			const key = KEYS[ev.code];

			if (key != undefined) {
				const text = ev.key.length == 1 ? MTY_StrToC(ev.key, _MTY.cbuf) : 0;

				if (MTY_CFunc(keyboard)(app, true, key, text, get_mods(ev)))
					ev.preventDefault();
			}
		});

		window.addEventListener('keyup', (ev) => {
			const key = KEYS[ev.code];

			if (key != undefined)
				if (MTY_CFunc(keyboard)(app, false, key, 0, get_mods(ev)))
					ev.preventDefault();
		});

		GL.canvas.addEventListener('dragover', (ev) => {
			ev.preventDefault();
		});

		window.addEventListener('blur', (ev) => {
			MTY_CFunc(focus)(app, false);
		});

		window.addEventListener('focus', (ev) => {
			MTY_CFunc(focus)(app, true);
		});

		GL.canvas.addEventListener('drop', (ev) => {
			ev.preventDefault();

			if (!ev.dataTransfer.items)
				return;

			for (let x = 0; x < ev.dataTransfer.items.length; x++) {
				if (ev.dataTransfer.items[x].kind == 'file') {
					let file = ev.dataTransfer.items[x].getAsFile();

					const reader = new FileReader();
					reader.addEventListener('loadend', (fev) => {
						if (reader.readyState == 2) {
							let buf = new Uint8Array(reader.result);
							let cmem = MTY_Alloc(buf.length);
							MTY_Memcpy(cmem, buf);
							MTY_CFunc(drop)(app, MTY_StrToC(file.name, _MTY.cbuf), cmem, buf.length);
							MTY_Free(cmem);
						}
					});
					reader.readAsArrayBuffer(file);
					break;
				}
			}
		});
	},
	web_raf: function (app, func, controller, opaque) {
		const step = () => {
			if (document.hasFocus())
				poll_gamepads(app, controller);

			const rect = GL.canvas.getBoundingClientRect();

			GL.canvas.width = scaled(rect.width);
			GL.canvas.height = scaled(rect.height);

			if (MTY_CFunc(func)(opaque)) {
				window.requestAnimationFrame(step);

			} else {
				_MTY.endFunc();
			}
		};

		window.requestAnimationFrame(step);
		throw 'MTY_AppRun halted execution';
	},
};


// WASI API

// https://github.com/WebAssembly/WASI/blob/master/phases/snapshot/docs.md

const FDS = {};
let ARG0 = '';
let FD_NUM = 64;
let FD_PREOPEN = false;

function append_buf_to_b64(b64, buf) {
	// FIXME This is a crude way to handle appending to an open file,
	// complex seek operations will break this

	const cur_buf = b64_to_buf(b64);
	const new_buf = new Uint8Array(cur_buf.length + buf.length);

	new_buf.set(cur_buf);
	new_buf.set(buf, cur_buf.length);

	return buf_to_b64(new_buf);
}

function arg_list() {
	const params = new URLSearchParams(window.location.search);
	const qs = params.toString();

	let plist = [ARG0];

	// TODO This would put each key/val pair as a separate arg
	// for (let p of params)
	// 	plist.push(p[0] + '=' + p[1]);

	//return plist;


	// For now treat the entire query string as argv[1]
	if (qs)
		plist.push(qs);

	return plist;
}

const WASI_API = {
	// Command line arguments
	args_get: function (argv, argv_buf) {
		const args = arg_list();
		for (let x = 0; x < args.length; x++) {
			MTY_StrToC(args[x], argv_buf);
			MTY_SetUint32(argv + x * 4, argv_buf);
			argv_buf += args[x].length + 1;
		}

		return 0;
	},
	args_sizes_get: function (argc, argv_buf_size) {
		const args = arg_list();

		MTY_SetUint32(argc, args.length);
		MTY_SetUint32(argv_buf_size, args.join(' ').length + 1);
		return 0;
	},

	// WASI preopened directory (/)
	fd_prestat_get: function (fd, path) {
		return !FD_PREOPEN ? 0 : 8;
	},
	fd_prestat_dir_name: function (fd, path, path_len) {
		if (!FD_PREOPEN) {
			MTY_StrToC('/', path);
			FD_PREOPEN = true;

			return 0;
		}

		return 28;
	},

	// Paths
	path_filestat_get: function (fd, flags, cpath, _0, filestat_out) {
		const path = MTY_StrToJS(cpath);
		if (localStorage[path]) {
			// We only need to return the size
			const buf = b64_to_buf(localStorage[path]);
			MTY_SetUint64(filestat_out + 32, buf.byteLength);
		}

		return 0;
	},
	path_open: function (fd, dir_flags, path, o_flags, _0, _1, _2, mode, fd_out) {
		const new_fd = FD_NUM++;
		MTY_SetUint32(fd_out, new_fd);

		FDS[new_fd] = {
			path: MTY_StrToJS(path),
			append: mode == 1,
			offset: 0,
		};

		return 0;
	},
	path_create_directory: function () {
		return 0;
	},
	path_remove_directory: function () {
		return 0;
	},
	path_unlink_file: function () {
		return 0;
	},
	path_readlink: function () {
	},

	// File descriptors
	fd_close: function (fd) {
		delete FDS[fd];
	},
	fd_fdstat_get: function () {
		return 0;
	},
	fd_fdstat_set_flags: function () {
	},
	fd_readdir: function () {
		return 8;
	},
	fd_seek: function (fd, offset, whence, offset_out) {
		return 0;
	},
	fd_read: function (fd, iovs, iovs_len, nread) {
		const finfo = FDS[fd];

		if (finfo && localStorage[finfo.path]) {
			const full_buf = b64_to_buf(localStorage[finfo.path]);

			let ptr = iovs;
			let cbuf = MTY_GetUint32(ptr);
			let cbuf_len = MTY_GetUint32(ptr + 4);
			let len = cbuf_len < full_buf.length ? cbuf_len : full_buf.length;

			let view = new Uint8Array(mem(), cbuf, cbuf_len);
			let slice = new Uint8Array(full_buf.buffer, 0, len);
			view.set(slice);

			MTY_SetUint32(nread, len);
		}

		return 0;
	},
	fd_write: function (fd, iovs, iovs_len, nwritten) {
		// Calculate full write size
		let len = 0;
		for (let x = 0; x < iovs_len; x++)
			len += MTY_GetUint32(iovs + x * 8 + 4);

		MTY_SetUint32(nwritten, len);

		// Create a contiguous buffer
		let offset = 0;
		let full_buf = new Uint8Array(len);
		for (let x = 0; x < iovs_len; x++) {
			let ptr = iovs + x * 8;
			let cbuf = MTY_GetUint32(ptr);
			let cbuf_len = MTY_GetUint32(ptr + 4);

			full_buf.set(new Uint8Array(mem(), cbuf, cbuf_len), offset);
			offset += cbuf_len;
		}

		// stdout
		if (fd == 1) {
			console.log(char_to_js(full_buf));

		// stderr
		} else if (fd == 2) {
			console.error(char_to_js(full_buf));

		// Filesystem
		} else if (FDS[fd]) {
			const finfo = FDS[fd];
			const cur_b64 = localStorage[finfo.path];

			if (cur_b64 && finfo.append) {
				localStorage[finfo.path] = append_buf_to_b64(cur_b64, full_buf);

			} else {
				localStorage[finfo.path] = buf_to_b64(full_buf, len);
			}

			finfo.offet += len;
		}

		return 0;
	},

	// Misc
	clock_time_get: function (id, precision, time_out) {
		MTY_SetUint64(time_out, Math.round(performance.now() * 1000.0 * 1000.0));
		return 0;
	},
	poll_oneoff: function (sin, sout, nsubscriptions, nevents) {
		return 0;
	},
	proc_exit: function () {
	},
};


// Entry

function supportsWASM() {
	try {
		if (typeof WebAssembly == 'object' && typeof WebAssembly.instantiate == 'function') {
			const module = new WebAssembly.Module(Uint8Array.of(0x0, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00));

			if (module instanceof WebAssembly.Module)
				return new WebAssembly.Instance(module) instanceof WebAssembly.Instance;
		}
	} catch (e) {}

	return false;
}

function supportsWebGL() {
	try {
		return document.createElement('canvas').getContext('webgl');
	} catch (e) {}

	return false;
}

async function MTY_Start(bin, userEnv, endFunc) {
	ARG0 = bin;

	if (!supportsWASM() || !supportsWebGL())
		return false;

	if (!userEnv)
		userEnv = {};

	if (endFunc)
		_MTY.endFunc = endFunc;

	// Set up full window canvas and webgl context
	const html = document.querySelector('html');
	html.style.width = '100%';
	html.style.height = '100%';
	html.style.margin = 0;

	const body = document.querySelector('body');
	body.style.width = '100%';
	body.style.height = '100%';
	body.style.background = 'black';
	body.style.overflow = 'hidden';
	body.style.margin = 0;

	const canvas = document.createElement('canvas');
	canvas.style.width = '100%';
	canvas.style.height = '100%';
	document.body.appendChild(canvas);

	GL = canvas.getContext('webgl', {depth: 0, antialias: 0, premultipliedAlpha: true});

	// Set up the clipboard
	CLIPBOARD = document.createElement('textarea');
	CLIPBOARD.style.position = 'absolute';
	CLIPBOARD.style.left = '-9999px';
	CLIPBOARD.autofocus = true;
	document.body.appendChild(CLIPBOARD);

	// Load keyboard map
	if (navigator.keyboard)
		_MTY.kbMap = await navigator.keyboard.getLayoutMap();

	// Fetch the wasm file as an ArrayBuffer
	const res = await fetch(bin);
	const buf = await res.arrayBuffer();

	// Create wasm instance (module) from the ArrayBuffer
	MODULE = await WebAssembly.instantiate(buf, {
		// Custom imports
		env: {
			...GL_API,
			...MTY_AUDIO_API,
			...MTY_WEB_API,
			...userEnv,
		},

		// Current version of WASI we're compiling against, 'wasi_snapshot_preview1'
		wasi_snapshot_preview1: {
			...WASI_API,
		},
	});

	// Execute the '_start' entry point, this will fetch args and execute the 'main' function
	try {
		MODULE.instance.exports._start();

	// We expect to catch the 'MTY_AppRun halted execution' exception
	// Otherwise look for an indication of unsupported WASM features
	} catch (e) {
		estr = e.toString();

		if (estr.search('MTY_AppRun') == -1)
			console.error(e);

		// This probably means the browser does not support WASM 64
		return estr.search('i64 not allowed') == -1;
	}

	return true;
}
