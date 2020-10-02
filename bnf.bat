@echo off
cd "C:\sjtwo-c"
scons && python nxp-programmer/flash.py
pause