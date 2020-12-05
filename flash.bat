@echo off
cd "C:\sjtwo-c"
set arg1=%1
python nxp-programmer/flash.py -i "_build_%1/%1.bin"