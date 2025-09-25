@echo off

set ProjectRoot=%CD%

if not exist build md build
pushd build

set CompileFlags=/nologo /std:c++14 /W4 /Zi /O2 /I%ProjectRoot%\include /FeWin32SmoothSizing

set cmd=cl %CompileFlags% %ProjectRoot%\src\main.cpp %ProjectRoot%\src\glad.c %ProjectRoot%\src\glad_wgl.c
echo %cmd%
%cmd%

popd
