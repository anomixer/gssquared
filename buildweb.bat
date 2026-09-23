@echo off
call C:\dev\MameWasm\emsdk\emsdk_env.bat
rem Reconfigure every time so changes to the Emscripten shell are picked up.
call emcmake cmake -B build-web -DGS2_WASM_SIMD=ON
rem Emscripten embeds shell.html during the final link. Clean the target so
rem shell-only edits cannot leave a stale build-web/GSSquared.html behind.
cmake --build build-web --target clean
cmake --build build-web --target GSSquared -j %NUMBER_OF_PROCESSORS%
