@echo off
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
   call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
) else (
   call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
)

if not exist build mkdir build
pushd build

set CompilerSwitches=%CompilerSwitches% /nologo /std:c17 /fp:fast /EHa- /FAs /FC /GF /GR- /GS- /Gs0x100000 /J /WX /Wall /X
set CompilerSwitches=%CompilerSwitches% /wd4702 /wd4101 /wd5045 /wd4820 /wd4701 /wd4213 /wd4242 /wd4244 /wd4201 /wd4061 /wd4062 /wd4127 /wd4214 /wd4189 /wd4100 /wd4094 /wd4200 /wd4018 /wd4710 /wd4706 /wd4703
set CompilerSwitches=%CompilerSwitches% /D_MSVC /D_X64 /D_DEBUG
set CompilerSwitches=%CompilerSwitches% /Od /Z7 /Oi
set LinkerSwitches=%LinkerSwitches% /wx /incremental:no /opt:ref /opt:icf /nodefaultlib /subsystem:console /stack:0x100000,0x100000 /machine:x64
set DLLCompilerSwitches=%DLLCompilerSwitches% /LD
set DLLLinkerSwitches=%DLLLinkerSwitches% /noimplib /noentry

if exist *.pdb del *.pdb > NUL 2> NUL
echo WAITING FOR PDB > lock.tmp

cl %CompilerSwitches% %DLLCompilerSwitches% /D_ASSEMBLER_MODULE /I ..\src\ ..\src\assembler\main.c /link %LinkerSwitches% %DLLLinkerSwitches% /pdb:Assembler_%random%.pdb /out:Assembler.dll

del lock.tmp
cl %CompilerSwitches% /D_PLATFORM_MODULE /I ..\src ..\src\platform\win32\entry.c /link %LinkerSwitches% /entry:Platform_Entry /out:Platform.exe

if exist *.obj del *.obj
if exist *.exp del *.exp
popd
exit /b 0