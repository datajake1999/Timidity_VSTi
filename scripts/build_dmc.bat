@echo off
set path=C:\DM\Bin;%path%
set include=C:\DM\Include
set lib=C:\DM\Lib
dmc -c -DVST_FORCE_DEPRECATED=0 -DGUI -I..\VST2_SDK -o @src.lst
rcc -32 ..\res\Timidity.rc
link /exetype:nt CPU.obj editor.obj guihelp.obj lock.obj main.obj midiprog.obj Timidity.obj Timidity_proc.obj queue.obj reaper.obj common.obj filter.obj instrum.obj mix.obj playmidi.obj readcfg.obj resample.obj tables.obj audioeffect.obj audioeffectx.obj vstplugmain.obj,Timidity.dll,,advapi32.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib shell32.lib user32.lib,..\res\vstplug.def,Timidity.res
del *.map *.obj *.res
