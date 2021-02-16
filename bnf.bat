@echo off
set arg1=%1
scons --project=%1 --no-float-format
python nxp-programmer/flash.py -i "_build_%1/%1.bin"