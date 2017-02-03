@echo off
mkdir ..\build
pushd ..\build
cl /Zi ..\src\CapsNumScroll.cpp shell32.lib user32.lib
popd


