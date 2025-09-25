# Responsive window resizing/moving on Windows

This program is written to show how a Windows app can be resized and moved while displaying animation at a high frame rate and without rendering artifacts. It is based off of the repo [JaiRenderThreadExample](https://github.com/CookedNick/JaiRenderThreadExample) by @CookedNick which was written in Jai. See that repo for a more detailed explaination of the problem and this particular solution.

The main branch is written in C++ and uses OpenGL. This branch is C which is almost exactly the same.

This was also an exercise for me to learn some basic Win32 and OpenGL so there may be better ways to do this or some of this might be bad. The only external code used was from [Glad](https://glad.dav1d.de/), of which the generated files are included.

## Requirements
- Visual Studio/MS Build Tools
- OpenGL 3.3+ compatible graphics card

## Building
From a MSVC enabled command prompt, from the root of the repo, run `build.bat`.
