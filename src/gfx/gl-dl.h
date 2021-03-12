// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#pragma once

#if defined(MTY_GL_ES)
	#define GL_SHADER_VERSION "#version 100\n"
#else
	#define GL_SHADER_VERSION "#version 110\n"
#endif


// Interface

#if defined MTY_GL_EXTERNAL

#if defined(MTY_GL_INCLUDE)
	#include MTY_GL_INCLUDE
#else
	#define GL_GLEXT_PROTOTYPES
	#include "glcorearb30.h"
#endif

#define gl_dl_global_init() true

#else

#include "glcorearb30.h"

static PFNGLGENFRAMEBUFFERSPROC         glGenFramebuffers;
static PFNGLDELETEFRAMEBUFFERSPROC      glDeleteFramebuffers;
static PFNGLBINDFRAMEBUFFERPROC         glBindFramebuffer;
static PFNGLBLITFRAMEBUFFERPROC         glBlitFramebuffer;
static PFNGLFRAMEBUFFERTEXTURE2DPROC    glFramebufferTexture2D;
static PFNGLENABLEPROC                  glEnable;
static PFNGLISENABLEDPROC               glIsEnabled;
static PFNGLDISABLEPROC                 glDisable;
static PFNGLVIEWPORTPROC                glViewport;
static PFNGLGETINTEGERVPROC             glGetIntegerv;
static PFNGLGETFLOATVPROC               glGetFloatv;
static PFNGLBINDTEXTUREPROC             glBindTexture;
static PFNGLDELETETEXTURESPROC          glDeleteTextures;
static PFNGLTEXPARAMETERIPROC           glTexParameteri;
static PFNGLGENTEXTURESPROC             glGenTextures;
static PFNGLTEXIMAGE2DPROC              glTexImage2D;
static PFNGLTEXSUBIMAGE2DPROC           glTexSubImage2D;
static PFNGLDRAWELEMENTSPROC            glDrawElements;
static PFNGLGETATTRIBLOCATIONPROC       glGetAttribLocation;
static PFNGLSHADERSOURCEPROC            glShaderSource;
static PFNGLBINDBUFFERPROC              glBindBuffer;
static PFNGLVERTEXATTRIBPOINTERPROC     glVertexAttribPointer;
static PFNGLCREATEPROGRAMPROC           glCreateProgram;
static PFNGLUNIFORM1IPROC               glUniform1i;
static PFNGLUNIFORM1FPROC               glUniform1f;
static PFNGLUNIFORM4IPROC               glUniform4i;
static PFNGLUNIFORM4FPROC               glUniform4f;
static PFNGLACTIVETEXTUREPROC           glActiveTexture;
static PFNGLDELETEBUFFERSPROC           glDeleteBuffers;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
static PFNGLBUFFERDATAPROC              glBufferData;
static PFNGLDELETESHADERPROC            glDeleteShader;
static PFNGLGENBUFFERSPROC              glGenBuffers;
static PFNGLCOMPILESHADERPROC           glCompileShader;
static PFNGLLINKPROGRAMPROC             glLinkProgram;
static PFNGLGETUNIFORMLOCATIONPROC      glGetUniformLocation;
static PFNGLCREATESHADERPROC            glCreateShader;
static PFNGLATTACHSHADERPROC            glAttachShader;
static PFNGLUSEPROGRAMPROC              glUseProgram;
static PFNGLGETSHADERIVPROC             glGetShaderiv;
static PFNGLDETACHSHADERPROC            glDetachShader;
static PFNGLDELETEPROGRAMPROC           glDeleteProgram;
static PFNGLCLEARPROC                   glClear;
static PFNGLCLEARCOLORPROC              glClearColor;
static PFNGLGETERRORPROC                glGetError;
static PFNGLGETSHADERINFOLOGPROC        glGetShaderInfoLog;
static PFNGLFINISHPROC                  glFinish;
static PFNGLSCISSORPROC                 glScissor;
static PFNGLBLENDFUNCPROC               glBlendFunc;
static PFNGLBLENDEQUATIONPROC           glBlendEquation;
static PFNGLUNIFORMMATRIX4FVPROC        glUniformMatrix4fv;
static PFNGLBLENDEQUATIONSEPARATEPROC   glBlendEquationSeparate;
static PFNGLBLENDFUNCSEPARATEPROC       glBlendFuncSeparate;
static PFNGLGETPROGRAMIVPROC            glGetProgramiv;


// Runtime open

static MTY_Atomic32 GL_DL_LOCK;
static bool GL_DL_INIT;

static bool gl_dl_global_init(void)
{
	MTY_GlobalLock(&GL_DL_LOCK);

	if (!GL_DL_INIT) {
		bool r = true;

		#define GL_DL_LOAD_SYM(name) \
			name = MTY_GLGetProcAddress(#name); \
			if (!name) {r = false; goto except;}

		GL_DL_LOAD_SYM(glGenFramebuffers);
		GL_DL_LOAD_SYM(glDeleteFramebuffers);
		GL_DL_LOAD_SYM(glBindFramebuffer);
		GL_DL_LOAD_SYM(glBlitFramebuffer);
		GL_DL_LOAD_SYM(glFramebufferTexture2D);
		GL_DL_LOAD_SYM(glEnable);
		GL_DL_LOAD_SYM(glIsEnabled);
		GL_DL_LOAD_SYM(glDisable);
		GL_DL_LOAD_SYM(glViewport);
		GL_DL_LOAD_SYM(glGetIntegerv);
		GL_DL_LOAD_SYM(glGetFloatv);
		GL_DL_LOAD_SYM(glBindTexture);
		GL_DL_LOAD_SYM(glDeleteTextures);
		GL_DL_LOAD_SYM(glTexParameteri);
		GL_DL_LOAD_SYM(glGenTextures);
		GL_DL_LOAD_SYM(glTexImage2D);
		GL_DL_LOAD_SYM(glTexSubImage2D);
		GL_DL_LOAD_SYM(glDrawElements);
		GL_DL_LOAD_SYM(glGetAttribLocation);
		GL_DL_LOAD_SYM(glShaderSource);
		GL_DL_LOAD_SYM(glBindBuffer);
		GL_DL_LOAD_SYM(glVertexAttribPointer);
		GL_DL_LOAD_SYM(glCreateProgram);
		GL_DL_LOAD_SYM(glUniform1i);
		GL_DL_LOAD_SYM(glUniform1f);
		GL_DL_LOAD_SYM(glUniform4i);
		GL_DL_LOAD_SYM(glUniform4f);
		GL_DL_LOAD_SYM(glActiveTexture);
		GL_DL_LOAD_SYM(glDeleteBuffers);
		GL_DL_LOAD_SYM(glEnableVertexAttribArray);
		GL_DL_LOAD_SYM(glBufferData);
		GL_DL_LOAD_SYM(glDeleteShader);
		GL_DL_LOAD_SYM(glGenBuffers);
		GL_DL_LOAD_SYM(glCompileShader);
		GL_DL_LOAD_SYM(glLinkProgram);
		GL_DL_LOAD_SYM(glGetUniformLocation);
		GL_DL_LOAD_SYM(glCreateShader);
		GL_DL_LOAD_SYM(glAttachShader);
		GL_DL_LOAD_SYM(glUseProgram);
		GL_DL_LOAD_SYM(glGetShaderiv);
		GL_DL_LOAD_SYM(glDetachShader);
		GL_DL_LOAD_SYM(glDeleteProgram);
		GL_DL_LOAD_SYM(glClear);
		GL_DL_LOAD_SYM(glClearColor);
		GL_DL_LOAD_SYM(glGetError);
		GL_DL_LOAD_SYM(glGetShaderInfoLog);
		GL_DL_LOAD_SYM(glFinish);
		GL_DL_LOAD_SYM(glScissor);
		GL_DL_LOAD_SYM(glBlendFunc);
		GL_DL_LOAD_SYM(glBlendEquation);
		GL_DL_LOAD_SYM(glUniformMatrix4fv);
		GL_DL_LOAD_SYM(glBlendEquationSeparate);
		GL_DL_LOAD_SYM(glBlendFuncSeparate);
		GL_DL_LOAD_SYM(glGetProgramiv);

		except:

		GL_DL_INIT = r;
	}

	MTY_GlobalUnlock(&GL_DL_LOCK);

	return GL_DL_INIT;
}

#endif
