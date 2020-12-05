@echo off
cd "C:\sjtwo-c"
set arg1=%1
scons --no-unit-test --project=%1 --no-float-format
