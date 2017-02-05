@echo off
mkdir ..\build
pushd ..\build
rc ..\src\CapsNumScroll.rc
move ..\src\CapsNumScroll.res .\
cl /FC /Zi ..\src\CapsNumScroll.cpp gdi32.lib shell32.lib user32.lib .\CapsNumScroll.res
popd


