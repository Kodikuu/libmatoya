## Overview

`libmatoya` is a lightweight, cross-platform, native application development library. The main goals of `libmatoya` are:
- Abstract platform differences into a single interface with consistent behavior
- Allow access to low level operating system and graphics API functionality
- Wrap old, cumbersome interfaces into modern, ergonomic interfaces
- Bundle common application tasks into a single dependency

The name comes from a character in [Final Fantasy](https://en.wikipedia.org/wiki/Final_Fantasy_(video_game)) who needs a crystal eye to see.

The development of this library is closely tied to [Merton](https://github.com/matoya/merton).

## Building

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
