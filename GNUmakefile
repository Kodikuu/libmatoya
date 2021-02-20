UNAME_S = $(shell uname -s)
ARCH = $(shell uname -m)
NAME = libmatoya
PREFIX = mty

.SUFFIXES: .vert .frag

.vert.h:
	@hexdump -ve '1/1 "0x%.2x,"' $< | (echo 'static const GLchar VERT[]={' && cat && echo '0x00};') > $@

.frag.h:
	@hexdump -ve '1/1 "0x%.2x,"' $< | (echo 'static const GLchar FRAG[]={' && cat && echo '0x00};') > $@

.m.o:
	$(CC) $(OCFLAGS)  -c -o $@ $<

OBJS = \
	src/compress.o \
	src/crypto.o \
	src/fs.o \
	src/json.o \
	src/log.o \
	src/memory.o \
	src/proc.o \
	src/sort.o \
	src/hash.o \
	src/list.o \
	src/queue.o \
	src/thread.o \
	src/gfx/gl.o \
	src/gfx/gl-ui.o \
	src/gfx/render.o \
	src/gfx/window-ctx.o

OBJS := $(OBJS) \
	src/unix/compress.o \
	src/unix/fs.o \
	src/unix/memory.o \
	src/unix/proc.o \
	src/unix/thread.o \
	src/unix/time.o

SHADERS = \
	src/gfx/shaders/gl/vs.h \
	src/gfx/shaders/gl/fs.h \
	src/gfx/shaders/gl/vsui.h \
	src/gfx/shaders/gl/fsui.h

INCLUDES = \
	-Isrc \
	-Isrc/unix \
	-Ideps

DEFS = \
	-D_POSIX_C_SOURCE=200112L

FLAGS = \
	-Wall \
	-Wextra \
	-Wshadow \
	-Wno-unused-parameter \
	-Wno-switch \
	-std=c99 \
	-fPIC

TEST_FLAGS = \
	-nodefaultlibs

ifdef DEBUG
FLAGS := $(FLAGS) -O0 -g
else
FLAGS := $(FLAGS) -O3 -fvisibility=hidden
endif

############
### WASM ###
############
ifdef WASM
WASI_SDK = $(HOME)/wasi-sdk-12.0

CC = $(WASI_SDK)/bin/clang --sysroot=$(WASI_SDK)/share/wasi-sysroot
AR = $(WASI_SDK)/bin/ar

ARCH := wasm32

OBJS := $(OBJS) \
	src/unix/web/gfx/gl-ctx.o \
	src/unix/web/window.o

DEFS := $(DEFS) \
	-DMTY_GLUI_CLEAR_ALPHA=0.0f \
	-DMTY_GL_EXTERNAL \
	-DMTY_GL_ES

TARGET = web
INCLUDES := $(INCLUDES) -Isrc/unix/web

else
#############
### LINUX ###
#############
ifeq ($(UNAME_S), Linux)

OBJS := $(OBJS) \
	src/unix/linux/crypto.o \
	src/unix/linux/aes-gcm.o \
	src/unix/linux/net/dtls.o \
	src/unix/linux/generic/gfx/gl-ctx.o \
	src/unix/linux/generic/hid.o \
	src/unix/linux/generic/audio.o \
	src/unix/linux/generic/window.o \
	src/unix/net/request.o \
	src/net/secure.o \
	src/net/http.o \
	src/net/async.o \
	src/net/gzip.o \
	src/net/net.o \
	src/net/tcp.o \
	src/net/ws.o

TEST_LIBS = \
	-lc \
	-lm \
	-ldl \
	-lpthread

TARGET = linux
INCLUDES := $(INCLUDES) -Isrc/unix/linux -Isrc/unix/linux/generic
endif

#############
### APPLE ###
#############
ifeq ($(UNAME_S), Darwin)

.SUFFIXES: .metal

.metal.h:
	hexdump -ve '1/1 "0x%.2x,"' $< | (echo 'static const char MTL_LIBRARY[]={' && cat && echo '0x00};') > $@

ifndef TARGET
TARGET = macosx
endif

ifndef ARCH
ARCH = x86_64
endif

ifeq ($(TARGET), macosx)
MIN_VER = 10.11

OBJS := $(OBJS) \
	src/unix/apple/macosx/hid.o \
	src/unix/apple/gfx/metal-ctx.o \
	src/unix/apple/gfx/gl-ctx.o

else
MIN_VER = 11.0
FLAGS := $(FLAGS) -fembed-bitcode
DEFS := $(DEFS) -DMTY_GL_ES
endif

# FIXME iOS / tvOS have to use CryptoKit or rejection from app store

OBJS := $(OBJS) \
	src/unix/apple/macosx/aes-gcm.o \
	src/unix/apple/gfx/metal.o \
	src/unix/apple/gfx/metal-ui.o \
	src/unix/apple/net/dtls.o \
	src/unix/apple/audio.o \
	src/unix/apple/crypto.o \
	src/unix/apple/$(TARGET)/window.o \
	src/unix/net/request.o \
	src/net/secure.o \
	src/net/http.o \
	src/net/async.o \
	src/net/gzip.o \
	src/net/net.o \
	src/net/tcp.o \
	src/net/ws.o

SHADERS := $(SHADERS) \
	src/unix/apple/gfx/shaders/metal/quad.h \
	src/unix/apple/gfx/shaders/metal/ui.h

DEFS := $(DEFS) \
	-DMTY_SSL_EXTERNAL \
	-DMTY_GL_EXTERNAL

TEST_LIBS = \
	-lc \
	-framework OpenGL \
	-framework AppKit \
	-framework QuartzCore \
	-framework Metal \
	-framework AudioToolbox

FLAGS := $(FLAGS) \
	-m$(TARGET)-version-min=$(MIN_VER) \
	-isysroot $(shell xcrun --sdk $(TARGET) --show-sdk-path) \
	-arch $(ARCH)

INCLUDES := $(INCLUDES) -Isrc/unix/apple -Isrc/unix/apple/$(TARGET)
endif
endif

CFLAGS = $(INCLUDES) $(DEFS) $(FLAGS)
OCFLAGS = $(CFLAGS) -fobjc-arc

all: clean-build clear $(SHADERS)
	make objs -j4

objs: $(OBJS)
	mkdir -p bin/$(TARGET)/$(ARCH)
	$(AR) -crs bin/$(TARGET)/$(ARCH)/$(NAME).a $(OBJS)

test: clean-build clear $(SHADERS)
	make objs-test -j4

objs-test: $(OBJS) src/test.o
	$(CC) -o $(PREFIX)-test $(TEST_FLAGS) $(OBJS) src/test.o $(TEST_LIBS)
	./$(PREFIX)-test

###############
### ANDROID ###
###############

### Downloads ###
# https://developer.android.com/ndk/downloads -> Put in ~/android-ndk-xxx

ANDROID_NDK = $(HOME)/android-ndk-r21e

android: clean clear $(SHADERS)
	@$(ANDROID_NDK)/ndk-build -j4 \
		NDK_PROJECT_PATH=. \
		APP_BUILD_SCRIPT=Android.mk \
		APP_OPTIM=release \
		APP_PLATFORM=android-26 \
		--no-print-directory \
		| grep -v 'fcntl(): Operation not supported'

clean: clean-build
	@rm -rf obj
	@rm -f $(SHADERS)

clean-build:
	@rm -rf bin
	@rm -f src/test.o
	@rm -f $(OBJS)
	@rm -f $(NAME).so
	@rm -f $(PREFIX)-test

clear:
	@clear
