# GIPS: The GLSL Image Processing System

An image processing application that applies filters
written in the OpenGL Shading Language (GLSL).

This means two things:

- Everything is processed on the GPU.
- The image is updated in realtime when parameters change.

Multiple filters can be combined into a pipeline.

GIPS uses standard GLSL fragment shaders
for all image processing operations,
with a few [customizations](ShaderFormat.md).

GIPS runs on Windows and Linux operating systems,
and quite possibly others too.

GIPS requires a GPU that's capable of OpenGL 3.3, and suitable drivers.
Every GPU made after 2007 should support that;
on Windows systems, however, the vendor drivers must be installed.
(The drivers installed automatically by Windows often don't support OpenGL.)



## Usage Tips

The usage of the program should be somewhat self-explanatory.
Here are some specific hints for the non-obvious things:

- Use drag & drop from a file manager to load an image into GIPS.
- The filters / shaders that are visible in the "Add Filter" menu
  are taken from the `shaders` subdirectory of the directory
  where the `gips`(`.exe`) executable is located.
  - Shader files must have one of the extensions `.gips`, `.glsl` or `.frag`
    to be recognized.
  - New shaders can be put there any time, they will appear immediately
    when the menu is opened the next time.
  - Alternatively, drag & drop a shader file (from _any_ directory)
    to add it to the filter pipeline.
  - See the "[Shader Format](ShaderFormat.md)" document
    for details on writing filters.
- The header bars for each filter have right-click context menus. Using these,
  - filters can be removed from the pipeline
  - filters can be moved up/down in the pipeline
  - filters from the `shaders` subdirectory can be added
    at any position in the pipeline
- Filters can be individually turned on and off
  using a button in their header bar.
- The "show" button in the filter header bar is used
  to set the point in the pipeline from which the output
  that is shown on-screen (and saved to the file) is taken from.
- Ctrl+click a parameter slider to enter a value with the keyboard.
  This way, it's also possible to input values outside of the slider's range.
- Press F5 to reload the shaders.
- Press Ctrl+F5 to reload the shaders and the input image.



## Limitations

Currently, GIPS is in "Minimum Viable Prototype" state; this means:

- filters can't change the image size
- filters always have exactly one input and one output
- filter pipeline is strictly linear, no node graphs
- the only supported channel format is RGBA
- no tiling: only images up to the maximum texture size supported by the GPU
  can be processed



## Building

First, ensure you have all the submodules checked out
(use `git clone --recursive` or `git submodule init --update`).

GIPS is written in C++11 and uses the CMake build system.

### Linux

Make sure that a compiler (GCC or Clang), CMake,
and the SDL2 development libraries are installed;
Ninja is also highly recommended.
At runtime, the `zenity` program must be available
in order to make the save/load dialogs work.

For example, on Debian/Ubuntu systems,
this should install everything that's needed:

    sudo apt install build-essential cmake libsdl2-dev ninja-build zenity

After that, you can just run `make`;
it creates a `_build` directory, runs CMake and finally Ninja.

The executable (`gips`) will be placed in the source directory,
*not* in the build directory.

### Windows

Visual Studio 2019 (any edition, including the IDE-less
[Build Tools](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=BuildTools&rel=16))
is required. Older versions of Visual Studio might also work, but are untested.
GCC / MinGW does *not* work currently due to an
[issue](https://github.com/samhocevar/portable-file-dialogs/issues/50)
in a third-party library.

A simple bootstrapping script is provided; just run `win32build.sh`,
and everything else should happen automatically:

- the Visual Studio installation is detected
  (for this, the [vswhere](https://github.com/microsoft/vswhere) tool
  is downloaded and used)
- a local copy of CMake is downloaded if there's no system-wide CMake installation
- a copy of [Ninja](https://ninja-build.org) is downloaded and used
- the SDL2 source code is downloaded
- CMake and Ninja are called

At the end of this process, a freshly-built `gips.exe` should have appeared
in the source directory. Since SDL2 is used as a static library,
it should not require any non-standard DLLs.

Using CMake directly, generating projects for Visual Studio is also possible,
but it's only really useful for Debug builds: due to a CMake limitation,
Release builds will be generated as console executables.


## Credits

GIPS is written by Martin J. Fiedler (<keyj@emphy.de>)

Used third-party software:

- [SDL2](https://www.libsdl.org)
  for window and OpenGL context creation and event handling
- [Dear ImGui](https://github.com/ocornut/imgui)
  for the user interface
- the OpenGL function loader has been generated with
  [GLAD](https://glad.dav1d.de/)
- Sean Barrett's [STB](https://github.com/nothings/stb) libs
  for image file I/O
- Sam Hocevar's [Portable File Dialogs](https://github.com/samhocevar/portable-file-dialogs)
- Timothy Lottes' FXAA algorithm is part of the example shaders
- the documentation is converted from Markdown to HTML using [Pandoc](https://pandoc.org/)
