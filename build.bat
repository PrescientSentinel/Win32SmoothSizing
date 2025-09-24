@echo off

set ProjectRoot=%CD%

if not exist build md build
pushd build

set CompileFlags=/nologo /W4 /Zi /O2 /I%ProjectRoot%\include /FeRenderThread

set cmd=cl %CompileFlags% %ProjectRoot%\src\main.c %ProjectRoot%\src\glad.c %ProjectRoot%\src\glad_wgl.c
echo %cmd%
%cmd%

popd
