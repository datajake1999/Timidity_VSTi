@echo off
g++ -c -DVST_FORCE_DEPRECATED=0 -DGUI -I..\VST2_SDK -O3 ..\src\*.cpp ..\timidity\*.c ..\VST2_SDK\public.sdk\source\vst2.x\*.cpp
windres ..\res\Timidity.rc resource.o
g++ *.o -s -static -shared -ladvapi32 -lcomctl32 -lcomdlg32 -lgdi32 -lkernel32 -lshell32 -luser32 -o Timidity.dll
del *.o
