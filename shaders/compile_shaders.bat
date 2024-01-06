@echo off
for /r %%i in (*.vert) do glslc %%i -o %%~ni_vert.spv
for /r %%i in (*.frag) do glslc %%i -o %%~ni_frag.spv