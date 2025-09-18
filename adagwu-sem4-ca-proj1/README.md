# OGLPROJ1
- [OGLPROJ1](#)
	- [Description](#description)
	- [Requirments](#requirments)
	- [How To Build](#how-to-build)
		- [Win32](#build-in-windows)
		- [Linux](#build-in-linux)
	- [How To Use](#how-to-use)

## Description

For computer animation course homework #1. Cmake project of motion controlling example in c++ using modern opengl libraries.

## Requirments

For building purpose.

- `cmake`
- `c++ compiler` (like Visual Studio 17 2022 or g++)
- `X11` (on linux platform only).

## How to Build

It is as easy as pressing one button (build/run) if you have `visual studio code` with cmake extension (https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools).

Optionally you can un-comment line 16 in `CMakeLists.txt` in order to get binaries straight in `bin/` (instead of `out/build/preset_name/config_name/`) folder for convinience.

Also you can edit `CMakePresets.json` to work with any other compiler (`visual studio` and `gcc` being already configured) and settings.

### Build in Windows

To build project, first generate the build files with command:

- `cmake --preset msvc1`

Then build with (will put binary in `"path/to/project/out/build/msvc1/Release/binary.exe"`):

- `cmake --build --preset msvc1-release`

### Build in Linux

Install tools:

- `sudo apt update`
- `sudo apt install -y build-essential cmake pkg-config git libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxfixes-dev libxi-dev libgl1-mesa-dev libudev-dev libxkbcommon-dev`

To build project, first generate the build files with command:

- `cmake --preset gnu1-release`

Then build with (will put binary in `"path/to/project/out/build/gnu1-release/bin/binary"`):

- `cmake --build out/build/gnu1-release`

## How to Use

Here is command options (you don't need library binaries for executable to run).

- -ot <type\> Orientation type: quat/quaternion/0 (default), euler/1
- -it <type\> Interpolation type: crspline/catmullrom/0 (default), bspline/1
- -kf <list> Keyframes, format: "x,y,z:e1,e2,e3;..." (Euler angles in degrees)
- -m <file>  File path, loads models with `.obj` extension (default: cube or `teapot.obj` file if it exist)
- -h, --help Show this help message\n";