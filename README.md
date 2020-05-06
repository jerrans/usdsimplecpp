# Simple C++ USD Sample

## What is this?
A C++ adaptation of the Usd Simple Shading Python tutorial here:
https://graphics.pixar.com/usd/docs/Simple-Shading-in-USD.html

But also includes creation of a window, OpenGL context and use of Hydra + Hd Storm renderer.

### Why?
I have a Vulkan application that uses compute shaders to produce renderable geometry, I also have a Vulkan renderer but I wanted to experiment with using Usd as the internal data model and then using Hd Storm to render it. So Vulkan compute to produce geometry then storing that in Usd and using Hydra + Hd Storm to render that.
This sample is just an excerpt that shows how to display a cube (optionally textured) onscreen using Usd + Hydra + Hd Storm.

## Getting
`git clone --recursive ssh://git@github.com/jerrans/usdsimplecpp`

## Building
Not tested on Windows or macOS but on Linux with GCC just run:

`sh build.sh`

Make sure to set your PYTHONPATH:

`export PYTHONPATH=your_source_directory/build/lib/python`

It builds USD first, then builds the usdSimpleCpp program.

## Running
From the build directory just run:

`./usdSimpleCpp`

You can supply an image on the command line and it will then texture the cube:

`./usdSimpleCpp myimage.png`

![Textured Example](/scr.png)

![non-Textured Example](/scr1.png)