@echo off
set path=C:\BCC55\Bin;%path%
set include=C:\BCC55\Include
set lib=C:\BCC55\Lib
bcc32 -q -c -DVST_FORCE_DEPRECATED=0 -DGUI -D_WIN32_WINNT=0x0400 -I..\include\pstdint;..\VST2_SDK -O2 -w-8004 -w-8057 @src.lst
brcc32 -fo Timidity.res ..\res\Timidity.rc
ilink32 -q /Tpd c0d32.obj CPU.obj editor.obj guihelp.obj lock.obj main.obj midiprog.obj queue.obj reaper.obj Timidity.obj Timidity_proc.obj common.obj filter.obj instrum.obj mix.obj playmidi.obj readcfg.obj readmidi.obj resample.obj tables.obj audioeffect.obj audioeffectx.obj vstplugmain.obj,Timidity.dll,,cw32mt.lib import32.lib,..\res\borland.def,Timidity.res
del *.ilc *.ild *.ilf *.ils *.map *.obj *.res *.tds
