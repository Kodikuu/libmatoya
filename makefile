TARGET = windows
ARCH = %%Platform%%
NAME = matoya
PREFIX = mty

.SUFFIXES : .ps4 .vs4 .ps3 .vs3 .vert .frag

.vs4.h:
	@fxc $(FXCFLAGS) /Fh $@ /T vs_4_0 /Vn $(*B) $<

.ps4.h:
	@fxc $(FXCFLAGS) /Fh $@ /T ps_4_0 /Vn $(*B) $<

.vs3.h:
	@fxc $(FXCFLAGS) /Fh $@ /T vs_3_0 /Vn $(*B) $<

.ps3.h:
	@fxc $(FXCFLAGS) /Fh $@ /T ps_3_0 /Vn $(*B) $<

.vert.h:
	@echo static const GLchar VERT[]={ > $@ && powershell "Format-Hex $< | %{$$_ -replace '^.*?   ',''} | %{$$_ -replace '  .*',' '} | %{$$_ -replace '(\w\w)','0x$$1,'}" >> $@ && echo 0x00}; >> $@

.frag.h:
	@echo static const GLchar FRAG[]={ > $@ && powershell "Format-Hex $< | %{$$_ -replace '^.*?   ',''} | %{$$_ -replace '  .*',' '} | %{$$_ -replace '(\w\w)','0x$$1,'}" >> $@ && echo 0x00}; >> $@

OBJS = \
	src\compress.obj \
	src\crypto.obj \
	src\fs.obj \
	src\json.obj \
	src\log.obj \
	src\memory.obj \
	src\hash.obj \
	src\list.obj \
	src\queue.obj \
	src\thread.obj \
	src\gfx\gl.obj \
	src\gfx\gl-ui.obj \
	src\gfx\render.obj \
	src\gfx\window-ctx.obj \
	src\net\secure.obj \
	src\net\http.obj \
	src\net\async.obj \
	src\net\gzip.obj \
	src\net\net.obj \
	src\net\tcp.obj \
	src\net\ws.obj

OBJS = $(OBJS) \
	src\windows\audio.obj \
	src\windows\compressw.obj \
	src\windows\cryptow.obj \
	src\windows\fsw.obj \
	src\windows\memoryw.obj \
	src\windows\proc.obj \
	src\windows\threadw.obj \
	src\windows\time.obj \
	src\windows\app.obj \
	src\windows\hid.obj \
	src\windows\aes-gcm.obj \
	src\windows\net\tls.obj \
	src\windows\net\request.obj \
	src\windows\gfx\gl-ctx.obj \
	src\windows\gfx\d3d9.obj \
	src\windows\gfx\d3d9-ui.obj \
	src\windows\gfx\d3d9-ctx.obj \
	src\windows\gfx\d3d11.obj \
	src\windows\gfx\d3d11-ui.obj \
	src\windows\gfx\d3d11-ctx.obj

SHADERS = \
	src\gfx\shaders\gl\vs.h \
	src\gfx\shaders\gl\fs.h \
	src\gfx\shaders\gl\vsui.h \
	src\gfx\shaders\gl\fsui.h \
	src\windows\gfx\shaders\d3d11\vs.h \
	src\windows\gfx\shaders\d3d11\ps.h \
	src\windows\gfx\shaders\d3d11\vsui.h \
	src\windows\gfx\shaders\d3d11\psui.h \
	src\windows\gfx\shaders\d3d9\vs.h \
	src\windows\gfx\shaders\d3d9\ps.h

INCLUDES = \
	-Isrc \
	-Isrc\windows \
	-Ideps

DEFS = \
	-DWIN32_LEAN_AND_MEAN \
	-DUNICODE

FXCFLAGS = \
	/O3 \
	/Ges \
	/nologo

FLAGS = \
	/W4 \
	/MT \
	/MP \
	/volatile:iso \
	/wd4100 \
	/wd4152 \
	/wd4201 \
	/wd4204 \
	/nologo

LIB_FLAGS = \
	/nologo

TEST_FLAGS = \
	$(LIB_FLAGS) \
	/nodefaultlib \
	/subsystem:console

TEST_LIBS = \
	libvcruntime.lib \
	libucrt.lib \
	libcmt.lib \
	kernel32.lib \
	user32.lib \
	shell32.lib \
	d3d11.lib \
	dxguid.lib \
	ole32.lib \
	uuid.lib \
	winmm.lib \
	bcrypt.lib \
	shlwapi.lib \
	ws2_32.lib \
	opengl32.lib

!IFDEF DEBUG
FLAGS = $(FLAGS) /Ob0 /Zi
TEST_FLAGS = $(TEST_FLAGS) /debug
!ELSE
FLAGS = $(FLAGS) /O2 /GS- /Gw
!ENDIF

CFLAGS = $(INCLUDES) $(DEFS) $(FLAGS)

all: clean-build clear $(SHADERS) $(OBJS)
	mkdir bin\$(TARGET)\$(ARCH)
	lib /out:bin\$(TARGET)\$(ARCH)\$(NAME).lib $(LIB_FLAGS) *.obj

test: all src\test.obj
	link /out:$(PREFIX)-test.exe $(TEST_FLAGS) *.obj $(TEST_LIBS)
	$(PREFIX)-test.exe

clean: clean-build
	@-del /q $(SHADERS) 2>nul

clean-build:
	@-rmdir /s /q bin 2>nul
	@-del /q *.obj 2>nul
	@-del /q *.lib 2>nul
	@-del /q *.pdb 2>nul
	@-del /q $(PREFIX)-test.exe 2>nul

clear:
	@cls
