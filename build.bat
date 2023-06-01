@echo off
cd /D "%~dp0"
if not defined DevEnvDir (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
)
cl /EHsc /Fe:"Timecard Processor.exe" main.c
mt.exe -manifest "Timecard Processor.exe.manifest" -outputresource:"Timecard Processor.exe";1
del *.obj *.manifest