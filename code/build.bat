@echo off

W:\link\misc\ctime.exe -begin link.ctime
set EntryFile=W:\link\code\win32_link.cpp
set VariableFlags=

set WarningFlags=-nologo -WX -W4 -wd4201 -wd4189 -wd4505 -wd4238 -wd4100 -wd4244

REM -FC = Full path of source file in diagnostics (so emacs can parse errors/warning)
REM -Zi = Creates debug information for Visual Studio debugger (Do I need to turn this off is release builds?)
REM -LD = Build DLL file
REM -Od = Turn off all optimizations
REM -incremental:no = To stop annyoing full link message

set CompilerFlags=%WarningFlags% %VariableFlags% -FC -Zi -Od
set LinkerFlags=-incremental:no
set ExternalLibraries=User32.lib Gdi32.lib Winmm.lib

pushd ..\..\build

DEL *.pdb > NUL 2> NUL

cl %CompilerFlags% ..\link\code\link.cpp -LD -link -incremental:no -PDB:link_%random%%random%.pdb -EXPORT:GameUpdateAndRender
cl %CompilerFlags% %EntryFile% %ExternalLibraries% /link %LinkerFlags%

popd
W:\xeno\misc\ctime.exe -end link.ctime