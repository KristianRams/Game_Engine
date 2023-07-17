@echo off

cl -MT -Gm- -Od -nologo -GR- -EHa- -Oi -FC -Fmwin32.map -Z7 win32.cpp /link -opt:ref user32.lib gdi32.lib dsound.lib dxguid.lib winmm.lib

