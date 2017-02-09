@echo off
mkdir ..\build
rc CapsNumScroll.rc
move CapsNumScroll.res ..\build
pushd ..\build
cl /FC /Zi ..\src\CapsNumScroll.cpp gdi32.lib shell32.lib user32.lib .\CapsNumScroll.res
popd


