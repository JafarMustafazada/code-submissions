# OGLPROJ1 
		- [Input Options](input-options)

## Description

For computer animation course homework #1. Cmake project of motion controlling example in c++ using modern opengl libraries.

## Requirments to build

- `cmake`
- `compiler` (like Visual Studio 17 2022 or g++)
- `X11` (on linux platform only).

## How To Use

It is as easy as pressing one button (build/run) if you have `visual studio code` with cmake extension (https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools). But in case of linux you also need to install some tools before pressing button. Following assumes you don't have `vs code`.

### Build in Win

To build project, first generate the build files with command:

- `cmake --preset msvc1`

Then build with (will put binary in `"path/to/project/out/build/preset_name/Release/binary.exe"`):

- `cmake --build --preset msvc1-release`

### Build in Linux

To install tools:

- `sudo apt update`
- `sudo apt install -y build-essential cmake pkg-config git libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxfixes-dev libxi-dev libgl1-mesa-dev libudev-dev libxkbcommon-dev`

To build project, first generate the build files with command:

- `cmake --preset gcc1-release`

Then build with (will put binary in `"path/to/project/out/build/preset_name/bin/binary"`):

- `cmake --build out/build/gcc1-release`

### Usage

Put binary in same directory as `teapot.obj` and run (you don't need library binaries for executable to run).

### Input Options
- -ot \<type> Orientation type: quat/quaternion/0 (default), euler/1
- -it \<type> Interpolation type: crspline/catmullrom/0 (default), bspline/b-spline/1
- -m \<file>  Load other model with .obj extension (default: teapot.obj, also can be data/model.obj)
- -h, --help Show this help message