## Overview

`libmatoya` is a lightweight, cross-platform, native application development library. The main goals of `libmatoya` are:
- Abstract platform differences into a single interface with consistent behavior
- Allow access to low level operating system and graphics API functionality
- Wrap old, cumbersome interfaces into modern, ergonomic interfaces
- Bundle common application tasks into a single dependency

The name comes from a character in [Final Fantasy](https://en.wikipedia.org/wiki/Final_Fantasy_(video_game)) who needs a crystal eye to see.

The development of this library is closely tied to [Merton](https://github.com/matoya/merton).

## Platform Support

| Platform | Minimum Version |
| -------- | --------------- |
| Windows  | 7               |
| Android  | API 26 (8.0)    |
| macOS    | 10.11           |
| iOS/tvOS | 13.0            |
| Linux    | `*`             |
| web      | `**`            |

`*` Linux doesn't have a minimum version per se, but relies on certain dependencies being present on the system at run time, i.e. `libssl` and `libX11`.

`**` The WASM (web) version requires WASM 64-bit support, which appeared in Chrome 86. Safari is not supported.

## Code

`libmatoya` is set up in a tree-like fashion where the code in commmon with all platforms is at the root of [`/src`](/src). From there, as you go further up the tree towards the leaves, the code becomes more platform specific. A major development goal is to move as much code as possible towards the root and maintain as little as possible towards the leaves. `libmatoya` never uses platform `#ifdef`s in its source, instead it uses the makefile to choose different paths through the tree depending on the platform.

- [`/src/windows`](/src/windows)
- [`/src/unix`](/src/unix)
    - [`/src/unix/web`](/src/unix/web)
    - [`/src/unix/apple`](/src/unix/apple)
        - [`/src/unix/apple/macosx`](/src/unix/apple/macosx)
        - [`/src/unix/apple/iphoneos`](/src/unix/apple/macosx)
    - [`/src/unix/linux`](/src/unix/linux)
        - [`/src/unix/linux/generic`](/src/unix/linux/generic)
        - [`/src/unix/linux/android`](/src/unix/linux/android)

The `libmatoya` interface can be considered a blend of basic window management (i.e. features of [`SDL`](https://github.com/libsdl-org/SDL) and [`GLFW`](https://github.com/glfw/glfw)), cross platform utility and convenience (audio playback, threads, queues, lists, filesystem helpers), and application management features that you might find in [`Electron`](https://github.com/electron/electron) or the browser's [`JavaScript API`](https://developer.mozilla.org/en-US/docs/Web/JavaScript). THe philosophy is that to be productive in developing a modern application, you usually need all of this stuff, so why not put it all in one place. Some notable features:

- Image compression/decompression routines
- Common cryptography tasks such as CRC32, SHA1, SHA256, HMAC, secure random number generation, and AES-GCM
- Simple filesystem heplers intended for basic IO on small files
- JSON parsing and construction
- Wrapped window and graphics context supporting `OpenGL`, `D3D9`, `D3D11`, and `Metal`
- Built in frame rendering with a variety of color formats inteded for video players and emulators
- Built in UI draw list rendering intended for output from systems such as imgui or nuklear
- Simple commonly used data structures: linked list, thread safe queue, and hash table
- Many threading features including readers-writer locks and thread pools
- HTTP/HTTPS and WebSockets using platform specific crypto libraries (`libssl` on Linux)

On Linux, `libmatoya` assumes an X11 environment and immediately looks for `libX11` and `libGL`. In order to use certain function related to cryptography and HTTP requests, `libssl` must be present on the system. Audio playback relies on `libasound` and gamepad support relies on `libudev`.

The WASM implementation is considered a "Unix" platform because of its build tools, the [WASI SDK](https://github.com/WebAssembly/wasi-sdk). Unlike most systems targeting the browser, `libmatoya` does not use Emscripten and instead implements support directly in [`matoya.js`](/src/unix/web/matoya.js).

## Dependencies

A major goal of `libmatoya` is to avoid third party dependencies. All libraries required are either guaranteed to be present on the target platforms or are compiled with `libmatoya` from the [`/deps`](/deps) directory. Thanks to:

- [`cJSON`](https://github.com/DaveGamble/cJSON): JSON support
- [`OpenGL`](https://github.com/KhronosGroup/OpenGL-Registry): OpenGL and OpenGLES headers
- [`miniz`](https://github.com/richgel999/miniz): `gzip` decompression for HTTP requests
- [`stb`](https://github.com/nothings/stb): Image compression/decompression on Unix platforms

On Linux, you are required only to link against the C standard library. All other dependencies are queried at run time with `dlopen`. These "system" dependencies all have a long history of ABI stability and wide package manager support on the major distros.

## Building

Currently `libmatoya` is changing frequently and is not intended to be distributed as a shared library. It will never use wrapped build systems like CMake, Visual Studio projects, or Automake. `libmatoya` ships with simple makefiles that output lean static libraries. The only public header you are required to include is [`matoya.h`](/src/matoya.h).

#### Windows
```shell
nmake
```
#### macOS / Linux
```shell
make
```
#### iPhone
```shell
# Device
make TARGET=iphoneos ARCH=arm64

# Simulator
make TARGET=iphonesimulator
```
#### Apple TV
```shell
# Device
make TARGET=appletvos ARCH=arm64

# Simulator
make TARGET=appletvsimulator
```
#### Web
```shell
# Must be built on a Unix environment, see the GNUmakefile for details
make WASM=1
```
#### Android
```shell
# Must be built on a Unix environment, see the GNUmakefile for details
make android
```
