## Overview

`libmatoya` is a cross-platform native app development library. It's a small static library and a [single header file](/src/matoya.h). Written mainly in C but uses Objective-C, JavaScript, and Java where necessary. No C++.

#### Features

- `MTY_App` is the top level object supporting multiple windows (`MTY_Window`)
- `OpenGL`, `D3D9`, `D3D11`, and `Metal` context creation
- System tray and notifications
- Mouse, keyboard, and extensive game controller support
- Mouse and keyboard window "grabbing"
- PNG cursor support
- Relative mouse mode
- Clipboard support
- Hotkeys
- Simple audio playback
- JSON parsing and construction
- Image compression/decompression
- Simple filesystem heplers intended for basic IO on small files
- Built in frame rendering intended for video players and emulators
- Built in UI draw list rendering intended for output from systems such as [`imgui`](https://github.com/ocornut/imgui) or [`nuklear`](https://github.com/Immediate-Mode-UI/Nuklear)
- Simple commonly used data structures: `MTY_List`, thread-safe `MTY_Queue`, and `MTY_Hash`
- Common cryptography tasks: CRC32, SHA1, SHA256, HMAC, AES-GCM, TLS/DTLS protocol
- Many threading features including readers-writer locks and thread pools
- HTTP/HTTPS and WebSockets using platform specific crypto libraries

The name comes from a character in [Final Fantasy](https://en.wikipedia.org/wiki/Final_Fantasy_(video_game)) who needs a crystal eye to see.

The development of this library is closely tied to [Merton](https://github.com/matoya/merton).

## Platform Support

| Platform | Minimum Version            |
| -------- | -------------------------- |
| Windows  | 7                          |
| Android  | API 26 (8.0)               |
| macOS    | 10.11                      |
| iOS/tvOS | *Coming soon!*             |
| Linux    | `*`                        |
| web      | `**` Chrome 86, Firefox 79 |

`*` Linux doesn't have a minimum version per se, but relies on certain dependencies being present on the system at run time.

`**` Safari is currently not supported.

## Dependencies

A major goal of `libmatoya` is to avoid third-party dependencies and wrap system dependencies wherever possible. This approach keeps `libmatoya` very small while relying on performant and well-tested libraries shipped with the target platforms. Required system libraries are guaranteed to be present per [Platform Support](#platform-support). Third-party dependencies are compiled with `libmatoya` from the [`/deps`](/deps) directory.

## Building

`libmatoya` changes frequently and is not intended to be distributed as a shared library. It will never use wrapped build systems like CMake, Visual Studio projects, or Automake. `libmatoya` ships with simple makefiles that output lean static libraries. The only public header you are required to include is [`matoya.h`](/src/matoya.h).

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
