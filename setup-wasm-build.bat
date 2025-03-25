@echo off
REM Amamoto Setup Script for WebAssembly Compilation
REM This script automates the process of setting up Emscripten and compiling the C++ code to WebAssembly

echo ===================================================
echo Amamoto WebAssembly Setup and Build Script
echo ===================================================
echo This script will help you:
echo 1. Install Emscripten SDK (if not already installed)
echo 2. Configure the build environment
echo 3. Compile the C++ code to WebAssembly
echo 4. Integrate the WebAssembly module with the web frontend
echo.

REM Check if git is installed
where git >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: Git is required but not found.
    echo Please install Git from https://git-scm.com/downloads and try again.
    exit /b 1
)

REM Check if CMake is installed
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake is required but not found.
    echo Please install CMake from https://cmake.org/download/ and try again.
    exit /b 1
)

echo Checking for Emscripten SDK...
REM Check if emsdk directory already exists
if exist emsdk (
    echo Emscripten SDK directory found.
) else (
    echo Cloning Emscripten SDK repository...
    git clone https://github.com/emscripten-core/emsdk.git
    if %ERRORLEVEL% NEQ 0 (
        echo Error: Failed to clone Emscripten SDK repository.
        exit /b 1
    )
)

echo.
echo Setting up Emscripten SDK...
cd emsdk
call emsdk install latest
if %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to install Emscripten SDK.
    cd ..
    exit /b 1
)

call emsdk activate latest
if %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to activate Emscripten SDK.
    cd ..
    exit /b 1
)

echo.
echo Setting up environment variables...
call emsdk_env.bat

cd ..

echo.
echo Creating build directory...
if not exist build mkdir build

echo.
echo Configuring build with CMake and Emscripten...
call emcmake cmake -B build
if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake configuration failed.
    exit /b 1
)

echo.
echo Building WebAssembly module...
call cmake --build build
if %ERRORLEVEL% NEQ 0 (
    echo Error: Build failed.
    exit /b 1
)

echo.
echo Checking if WebAssembly files were generated...
if exist build\amamoto_module.js (
    echo WebAssembly files generated successfully!
) else (
    echo Error: WebAssembly files were not generated.
    exit /b 1
)

echo.
echo ===================================================
echo Build completed successfully!
echo.
echo The WebAssembly module has been compiled and copied to:
echo   - src/web/public/amamoto_module.js
echo   - src/web/public/amamoto_module.wasm
echo.
echo To use the WebAssembly module instead of the JavaScript implementation:
echo 1. Edit src/web/public/simulation.js to import from wasm-integration.js
echo 2. Restart your development server
echo ===================================================