@echo off

set DebugBuild=1

set UTEST_Path=%~dp0\utest\
set STL_Path=%~dp0\stl\

set IncludePath=-I%UTEST_Path% -I%STL_Path%

IF NOT EXIST %STL_Path%bin mkdir %STL_Path%bin

set CompileFlags=-nologo -D_HAS_EXCEPTIONS=0 -ZI

IF %DebugBuild% == 1 (
set CompileFlags=%CompileFlags% -Od
) ELSE (
set CompileFlags=%CompileFlags% -O2
)

pushd %STL_Path%bin
cl %CompileFlags% %IncludePath% %STL_Path%tests\stl_tests.cpp -link -out:stl_tests.exe
cl %CompileFlags% %IncludePath% %STL_Path%tests\stl_performance_tests.cpp -link -out:stl_performance_tests.exe
popd