@echo off

set ProjectRoot=%CD%

if not exist build md build
pushd build

set Libs=user32.lib gdi32.lib opengl32.lib
set CompileFlags=/nologo /W4 /Zi /I%ProjectRoot%\include /FeRenderThread
@REM /DEBUG for Linker to combine debug info from multiple object files
set LinkFlags=%Libs%

cl %CompileFlags% %ProjectRoot%\src\main.cpp %ProjectRoot%\src\glad.c %ProjectRoot%\src\glad_wgl.c /link %LinkFlags%
popd
