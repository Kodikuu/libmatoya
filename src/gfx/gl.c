// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "gfx/mod.h"
GFX_PROTOTYPES(_gl_)

#include <stdio.h>

#include "gl-dl.h"
#include "gfx/viewport.h"
#include "shaders/gl/vs.h"
#include "shaders/gl/fs.h"

#define NUM_STAGING 3

struct gfx_gl_rtv {
	GLenum format;
	GLuint texture;
	GLuint fb;
	uint32_t w;
	uint32_t h;
};

struct gfx_gl {
	MTY_ColorFormat format;
	struct gfx_gl_rtv staging[NUM_STAGING];

	GLuint vs;
	GLuint fs;
	GLuint prog;
	GLuint vb;
	GLuint eb;

	GLuint loc_tex[NUM_STAGING];
	GLuint loc_pos;
	GLuint loc_uv;
	GLuint loc_fcb;
	GLuint loc_icb;

	float scale;
};

static void gfx_gl_log_shader_errors(GLuint shader)
{
	GLint n = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &n);

	if (n > 0) {
		char *log = MTY_Alloc(n, 1);

		glGetShaderInfoLog(shader, n, NULL, log);
		MTY_Log("%s", log);
		MTY_Free(log);
	}
}

struct gfx *gfx_gl_create(MTY_Device *device)
{
	if (!gl_dl_global_init())
		return NULL;

	struct gfx_gl *ctx = MTY_Alloc(1, sizeof(struct gfx_gl));

	bool r = true;

	GLint status = GL_FALSE;
	const GLchar *vs[2] = {GL_SHADER_VERSION, VERT};
	ctx->vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(ctx->vs, 2, vs, NULL);
	glCompileShader(ctx->vs);
	glGetShaderiv(ctx->vs, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		gfx_gl_log_shader_errors(ctx->vs);
		r = false;
		goto except;
	}

	const GLchar *fs[2] = {GL_SHADER_VERSION, FRAG};
	ctx->fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(ctx->fs, 2, fs, NULL);
	glCompileShader(ctx->fs);
	glGetShaderiv(ctx->fs, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		gfx_gl_log_shader_errors(ctx->fs);
		r = false;
		goto except;
	}

	ctx->prog = glCreateProgram();
	glAttachShader(ctx->prog, ctx->vs);
	glAttachShader(ctx->prog, ctx->fs);
	glLinkProgram(ctx->prog);

	glGetProgramiv(ctx->prog, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		MTY_Log("Program failed to link");
		r = false;
		goto except;
	}

	ctx->loc_pos = glGetAttribLocation(ctx->prog, "position");
	ctx->loc_uv = glGetAttribLocation(ctx->prog, "texcoord");
	ctx->loc_fcb = glGetUniformLocation(ctx->prog, "fcb");
	ctx->loc_icb = glGetUniformLocation(ctx->prog, "icb");

	for (uint8_t x = 0; x < NUM_STAGING; x++) {
		char name[32];
		snprintf(name, 32, "tex%u", x);
		ctx->loc_tex[x] = glGetUniformLocation(ctx->prog, name);
	}

	glGenBuffers(1, &ctx->vb);
	GLfloat vertices[] = {
		-1.0f,  1.0f,	// Position 0
		 0.0f,  0.0f,	// TexCoord 0
		-1.0f, -1.0f,	// Position 1
		 0.0f,  1.0f,	// TexCoord 1
		 1.0f, -1.0f,	// Position 2
		 1.0f,  1.0f,	// TexCoord 2
		 1.0f,  1.0f,	// Position 3
		 1.0f,  0.0f	// TexCoord 3
	};

	glBindBuffer(GL_ARRAY_BUFFER, ctx->vb);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &ctx->eb);
	GLshort elements[] = {
		0, 1, 2,
		2, 3, 0
	};

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx->eb);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	GLenum e = glGetError();
	if (e != GL_NO_ERROR) {
		MTY_Log("'glGetError' returned %d", e);
		r = false;
		goto except;
	}

	except:

	if (!r)
		gfx_gl_destroy((struct gfx **) &ctx);

	return (struct gfx *) ctx;
}

static void gfx_gl_rtv_destroy(struct gfx_gl_rtv *rtv)
{
	if (rtv->texture) {
		glDeleteTextures(1, &rtv->texture);
		rtv->texture = 0;
	}
}

static void gfx_gl_rtv_refresh(struct gfx_gl_rtv *rtv, GLint internal, GLenum format, uint32_t w, uint32_t h)
{
	if (!rtv->texture || rtv->w != w || rtv->h != h || rtv->format != format) {
		gfx_gl_rtv_destroy(rtv);

		glGenTextures(1, &rtv->texture);
		glBindTexture(GL_TEXTURE_2D, rtv->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0, format, GL_UNSIGNED_BYTE, NULL);

		rtv->w = w;
		rtv->h = h;
		rtv->format = format;
	}
}

static void gfx_gl_reload_textures(struct gfx_gl *ctx, const void *image, const MTY_RenderDesc *desc)
{
	switch (desc->format) {
		case MTY_COLOR_FORMAT_RGBA: {
			gfx_gl_rtv_refresh(&ctx->staging[0], GL_RGBA8, GL_RGBA, desc->imageWidth, desc->cropHeight);
			glBindTexture(GL_TEXTURE_2D, ctx->staging[0].texture);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, desc->imageWidth, desc->cropHeight, GL_RGBA, GL_UNSIGNED_BYTE, image);
			break;
		}
		case MTY_COLOR_FORMAT_NV12:
		case MTY_COLOR_FORMAT_NV16: {
			uint32_t div = desc->format == MTY_COLOR_FORMAT_NV12 ? 2 : 1;

			// Y
			gfx_gl_rtv_refresh(&ctx->staging[0], GL_R8, GL_RED, desc->imageWidth, desc->cropHeight);
			glBindTexture(GL_TEXTURE_2D, ctx->staging[0].texture);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, desc->imageWidth, desc->cropHeight, GL_RED, GL_UNSIGNED_BYTE, image);

			// UV
			gfx_gl_rtv_refresh(&ctx->staging[1], GL_RG8, GL_RG, desc->imageWidth / 2, desc->cropHeight / div);
			glBindTexture(GL_TEXTURE_2D, ctx->staging[1].texture);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, desc->imageWidth / 2, desc->cropHeight / div, GL_RG, GL_UNSIGNED_BYTE, (uint8_t *) image + desc->imageWidth * desc->imageHeight);
			break;
		}
		case MTY_COLOR_FORMAT_I420:
		case MTY_COLOR_FORMAT_I444: {
			uint32_t div = desc->format == MTY_COLOR_FORMAT_I420 ? 2 : 1;

			// Y
			gfx_gl_rtv_refresh(&ctx->staging[0], GL_R8, GL_RED, desc->imageWidth, desc->cropHeight);
			glBindTexture(GL_TEXTURE_2D, ctx->staging[0].texture);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, desc->imageWidth, desc->cropHeight, GL_RED, GL_UNSIGNED_BYTE, image);

			// U
			uint8_t *p = (uint8_t *) image + desc->imageWidth * desc->imageHeight;
			gfx_gl_rtv_refresh(&ctx->staging[1], GL_R8, GL_RED, desc->imageWidth / div, desc->cropHeight / div);
			glBindTexture(GL_TEXTURE_2D, ctx->staging[1].texture);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, desc->imageWidth / div, desc->cropHeight / div, GL_RED, GL_UNSIGNED_BYTE, p);

			// V
			p += (desc->imageWidth / div) * (desc->imageHeight / div);
			gfx_gl_rtv_refresh(&ctx->staging[2], GL_R8, GL_RED, desc->imageWidth / div, desc->cropHeight / div);
			glBindTexture(GL_TEXTURE_2D, ctx->staging[2].texture);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, desc->imageWidth / div, desc->cropHeight / div, GL_RED, GL_UNSIGNED_BYTE, p);
			break;
		}
	}
}

bool gfx_gl_render(struct gfx *gfx, MTY_Device *device, MTY_Context *context,
	const void *image, const MTY_RenderDesc *desc, MTY_Texture *dest)
{
	struct gfx_gl *ctx = (struct gfx_gl *) gfx;
	GLuint _dest = dest ? *((GLuint *) dest) : 0;

	// Don't do anything until we have real data
	if (desc->format != MTY_COLOR_FORMAT_UNKNOWN) {
		ctx->scale = (float) desc->cropWidth / (float) desc->imageWidth;
		ctx->format = desc->format;
	}

	if (ctx->format == MTY_COLOR_FORMAT_UNKNOWN)
		return true;

	// Refresh staging texture dimensions
	gfx_gl_reload_textures(ctx, image, desc);

	// Viewport
	float vpx, vpy, vpw, vph;
	mty_viewport(desc->cropWidth, desc->cropHeight, desc->viewWidth, desc->viewHeight,
		desc->aspectRatio, desc->scale, &vpx, &vpy, &vpw, &vph);
	glViewport(lrint(vpx), lrint(vpy), lrint(vpw), lrint(vph));

	// Begin render pass (set destination texture if available)
	if (_dest)
		glBindFramebuffer(GL_FRAMEBUFFER, _dest);

	// Context state, set vertex and fragment shaders
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glUseProgram(ctx->prog);

	// Vertex shader
	glBindBuffer(GL_ARRAY_BUFFER, ctx->vb);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx->eb);
	glEnableVertexAttribArray(ctx->loc_pos);
	glEnableVertexAttribArray(ctx->loc_uv);
	glVertexAttribPointer(ctx->loc_pos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glVertexAttribPointer(ctx->loc_uv,  2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *) (2 * sizeof(GLfloat)));

	// Clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Fragment shader
	for (uint8_t x = 0; x < NUM_STAGING; x++) {
		if (ctx->staging[x].texture) {
			GLint filter = desc->filter == MTY_FILTER_NEAREST ? GL_NEAREST : GL_LINEAR;

			glActiveTexture(GL_TEXTURE0 + x);
			glBindTexture(GL_TEXTURE_2D, ctx->staging[x].texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glUniform1i(ctx->loc_tex[x], x);
		}
	}

	// Uniforms
	glUniform4f(ctx->loc_fcb, ctx->scale, (GLfloat) desc->cropWidth, (GLfloat) desc->cropHeight, vph);
	glUniform3i(ctx->loc_icb, desc->filter, desc->effect, ctx->format);

	// Draw
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

	return true;
}

void gfx_gl_destroy(struct gfx **gfx)
{
	if (!gfx || !*gfx)
		return;

	struct gfx_gl *ctx = (struct gfx_gl *) *gfx;

	for (uint8_t x = 0; x < NUM_STAGING; x++)
		gfx_gl_rtv_destroy(&ctx->staging[x]);

	if (ctx->vb)
		glDeleteBuffers(1, &ctx->vb);

	if (ctx->eb)
		glDeleteBuffers(1, &ctx->eb);

	if (ctx->prog) {
		if (ctx->vs)
			glDetachShader(ctx->prog, ctx->vs);

		if (ctx->fs)
			glDetachShader(ctx->prog, ctx->fs);
	}

	if (ctx->vs)
		glDeleteShader(ctx->vs);

	if (ctx->fs)
		glDeleteShader(ctx->fs);

	if (ctx->prog)
		glDeleteProgram(ctx->prog);

	MTY_Free(ctx);
	*gfx = NULL;
}


// State

struct gfx_gl_state {
	GLint array_buffer;
	GLenum active_texture;
	GLint program;
	GLint texture;
	GLint viewport[4];
	GLint scissor_box[4];
	GLenum blend_src_rgb;
	GLenum blend_dst_rgb;
	GLenum blend_src_alpha;
	GLenum blend_dst_alpha;
	GLenum blend_equation_rgb;
	GLenum blend_equation_alpha;
	GLboolean blend;
	GLboolean cull_face;
	GLboolean depth_test;
	GLboolean scissor_test;
};

void *gfx_gl_get_state(MTY_Device *device, MTY_Context *_context)
{
	if (!gl_dl_global_init())
		return NULL;

	struct gfx_gl_state *s = MTY_Alloc(1, sizeof(struct gfx_gl_state));

	glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint *) &s->active_texture);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &s->array_buffer);
	glGetIntegerv(GL_CURRENT_PROGRAM, &s->program);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &s->texture);
	glGetIntegerv(GL_VIEWPORT, s->viewport);
	glGetIntegerv(GL_SCISSOR_BOX, s->scissor_box);
	glGetIntegerv(GL_BLEND_SRC_RGB, (GLint *) &s->blend_src_rgb);
	glGetIntegerv(GL_BLEND_DST_RGB, (GLint *) &s->blend_dst_rgb);
	glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint *) &s->blend_src_alpha);
	glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint *) &s->blend_dst_alpha);
	glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint *) &s->blend_equation_rgb);
	glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint *) &s->blend_equation_alpha);
	s->blend = glIsEnabled(GL_BLEND);
	s->cull_face = glIsEnabled(GL_CULL_FACE);
	s->depth_test = glIsEnabled(GL_DEPTH_TEST);
	s->scissor_test = glIsEnabled(GL_SCISSOR_TEST);

	return s;
}

static void gfx_gl_enable(GLenum cap, bool enable)
{
	if (enable) {
		glEnable(cap);

	} else {
		glDisable(cap);
	}
}

void gfx_gl_set_state(MTY_Device *device, MTY_Context *_context, void *state)
{
	if (!gl_dl_global_init())
		return;

	struct gfx_gl_state *s = state;

	glUseProgram(s->program);
	glBindTexture(GL_TEXTURE_2D, s->texture);
	glActiveTexture(s->active_texture);
	glBindBuffer(GL_ARRAY_BUFFER, s->array_buffer);
	glBlendEquationSeparate(s->blend_equation_rgb, s->blend_equation_alpha);
	glBlendFuncSeparate(s->blend_src_rgb, s->blend_dst_rgb, s->blend_src_alpha, s->blend_dst_alpha);
	gfx_gl_enable(GL_BLEND, s->blend);
	gfx_gl_enable(GL_CULL_FACE, s->cull_face);
	gfx_gl_enable(GL_DEPTH_TEST, s->depth_test);
	gfx_gl_enable(GL_SCISSOR_TEST, s->scissor_test);
	glViewport(s->viewport[0], s->viewport[1], s->viewport[2], s->viewport[3]);
	glScissor(s->scissor_box[0], s->scissor_box[1], s->scissor_box[2], s->scissor_box[3]);
}

void gfx_gl_free_state(void **state)
{
	if (!state || !*state)
		return;

	struct gfx_gl_state *s = *state;

	MTY_Free(s);
	*state = NULL;
}


// Misc

void gfx_gl_finish(void)
{
	if (!gl_dl_global_init())
		return;

	glFinish();
}
