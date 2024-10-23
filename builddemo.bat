@ECHO OFF
set path="C:\Program Files\7-Zip";"C:\Program Files\Inno Setup 5"
cd..
7z a -tzip Timidity_VSTi\output\Timidity_VSTi-src.zip @Timidity_VSTi\zip-src.lst
cd Timidity_VSTi
call "C:\Program Files\Microsoft Visual Studio 8\VC\bin\vcvars32.bat"
call "C:\Program Files\Microsoft Platform SDK\SetEnv.Cmd" /SRV32 /RETAIL
cl /nologo -c /D _CRT_SECURE_NO_DEPRECATE /D VST_FORCE_DEPRECATED=0 /D DEMO /D GUI /I ".\include\msinttypes" /I ".\VST2_SDK" /O2 src\*.cpp timidity\*.c VST2_SDK\public.sdk\source\vst2.x\*.cpp
rc res\Timidity.rc
link *.obj res\Timidity.res advapi32.lib bufferoverflowU.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib shell32.lib user32.lib /nologo /dll /def:".\res\vstplug.def" /OUT:Timidity_X86.dll
del *.obj
call "C:\Program Files\Microsoft Platform SDK\SetEnv.Cmd" /X64 /RETAIL
cl /nologo -c /D _CRT_SECURE_NO_DEPRECATE /D VST_FORCE_DEPRECATED=0 /D DEMO /D GUI /I ".\include\msinttypes" /I ".\VST2_SDK" /O2 src\*.cpp timidity\*.c VST2_SDK\public.sdk\source\vst2.x\*.cpp
link *.obj res\Timidity.res advapi32.lib bufferoverflowU.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib shell32.lib user32.lib /nologo /dll /def:".\res\vstplug.def" /OUT:Timidity_X64.dll
del *.obj
call "C:\Program Files\Microsoft Platform SDK\SetEnv.Cmd" /SRV64 /RETAIL
cl /nologo -c /D _CRT_SECURE_NO_DEPRECATE /D VST_FORCE_DEPRECATED=0 /D DEMO /D GUI /I ".\include\msinttypes" /I ".\VST2_SDK" /O2 src\*.cpp timidity\*.c VST2_SDK\public.sdk\source\vst2.x\*.cpp
link *.obj res\Timidity.res advapi32.lib bufferoverflowU.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib shell32.lib user32.lib /nologo /dll /def:".\res\vstplug.def" /OUT:Timidity_IA64.dll
del *.exp *.lib *.obj
del res\Timidity.res
iscc /Qp "install.iss"
7z a -tzip output\Timidity_VSTi.zip @zip.lst
del *.dll
