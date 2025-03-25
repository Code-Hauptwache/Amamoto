@echo off
REM Amamoto WebAssembly Build Script for Simplified Implementation
REM This script builds only the simplified traffic simulation without template issues

echo ===================================================
echo Building Simplified WebAssembly Module
echo ===================================================

REM Check if Emscripten SDK is set up
if not exist emsdk (
    echo Error: Emscripten SDK is not set up.
    echo Please run setup-wasm-build.bat first to install the Emscripten SDK.
    exit /b 1
)

REM Set up Emscripten environment
echo Setting up Emscripten environment...
cd emsdk
call emsdk_env.bat
cd ..

REM Create build directory
echo Creating build directory for simplified module...
if not exist build-simple mkdir build-simple

REM Configure and build
echo Configuring build with CMake and Emscripten...
call emcmake cmake -B build-simple -S src/simulation/wasm -G "MinGW Makefiles"

echo Building WebAssembly module...
call cmake --build build-simple

REM Check if the build was successful
if %ERRORLEVEL% NEQ 0 (
    echo Build failed.
    exit /b 1
)

REM Copy output files to web directory
echo Copying files to web directory...
if exist build-simple\amamoto_module.js (
    copy build-simple\amamoto_module.js src\web\public\
    copy build-simple\amamoto_module.wasm src\web\public\
) else (
    echo Warning: WebAssembly files were not found in expected location
)

echo ===================================================
echo Build completed!
echo ===================================================
echo The simplified WebAssembly module has been compiled and copied to:
echo   - src/web/public/amamoto_module.js
echo   - src/web/public/amamoto_module.wasm
echo.
echo To use the WebAssembly module, update simulation.js as described in WASM_MIGRATION_GUIDE.md
echo ===================================================