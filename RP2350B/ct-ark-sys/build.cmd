@echo off
cls
del ct-ark2.lst 
.\AILZ80ASM -i ct-ark2.z80 -f -bin ct-ark2.sys -lst -lm full -ts 4
rem .\AILZ80ASM -i ct-ark.z80 -f -bin ct-ark2.sys

@rem copy ct-ark.sys ct-ark.rom /y

pause
