@echo off
cd /D "%~dp0"
if not defined DevEnvDir (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
)
cl /EHsc test.c
echo:
echo **********************************************************************
test.exe
echo **********************************************************************
echo:
del *.obj *.manifest test.exe
pause