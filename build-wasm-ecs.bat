@echo off
REM Amamoto WebAssembly ECS Build Script
REM This script builds the ECS-based traffic simulation

echo ===================================================
echo Building ECS WebAssembly Module
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
echo Creating build directory for ECS module...
if not exist build-ecs mkdir build-ecs

REM Create standalone CMakeLists.txt for the ECS module
echo Creating CMakeLists.txt for ECS module...

echo cmake_minimum_required(VERSION 3.10) > build-ecs\CMakeLists.txt
echo project(amamoto_ecs_simulation) >> build-ecs\CMakeLists.txt
echo. >> build-ecs\CMakeLists.txt
echo set(CMAKE_CXX_STANDARD 17) >> build-ecs\CMakeLists.txt
echo set(CMAKE_CXX_STANDARD_REQUIRED ON) >> build-ecs\CMakeLists.txt
echo. >> build-ecs\CMakeLists.txt
echo if(EMSCRIPTEN) >> build-ecs\CMakeLists.txt
echo     set(CMAKE_EXECUTABLE_SUFFIX ".js") >> build-ecs\CMakeLists.txt
echo     add_compile_options(-sWASM=1 -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_RUNTIME_METHODS=['UTF8ToString'] -fexceptions -frtti) >> build-ecs\CMakeLists.txt
echo     set(LINK_FLAGS "-sEXPORT_ES6=1 -sALLOW_MEMORY_GROWTH=1 -sMODULARIZE=1 -sEXPORT_NAME=AmattoECSSimulation -sNO_EXIT_RUNTIME=1 -sASSERTIONS=1") >> build-ecs\CMakeLists.txt
echo endif() >> build-ecs\CMakeLists.txt
echo. >> build-ecs\CMakeLists.txt
echo add_executable(amamoto_ecs >> build-ecs\CMakeLists.txt
echo     %CD%\src\simulation\wasm\WasmECS.cpp >> build-ecs\CMakeLists.txt
echo     %CD%\src\simulation\wasm\WasmRoadNetwork.cpp >> build-ecs\CMakeLists.txt
echo     %CD%\src\simulation\wasm\WasmTrafficSimulation.cpp >> build-ecs\CMakeLists.txt
echo     %CD%\src\simulation\wasm\wasm_bindings_ecs.cpp >> build-ecs\CMakeLists.txt
echo ) >> build-ecs\CMakeLists.txt
echo. >> build-ecs\CMakeLists.txt
echo target_include_directories(amamoto_ecs PUBLIC %CD%\src) >> build-ecs\CMakeLists.txt
echo. >> build-ecs\CMakeLists.txt
echo if(EMSCRIPTEN) >> build-ecs\CMakeLists.txt
echo     set_target_properties(amamoto_ecs PROPERTIES LINK_FLAGS "${LINK_FLAGS}") >> build-ecs\CMakeLists.txt
echo     target_link_libraries(amamoto_ecs "-lembind") >> build-ecs\CMakeLists.txt
echo endif() >> build-ecs\CMakeLists.txt

REM Run CMake to configure the build
echo Configuring build with CMake and Emscripten...
cd build-ecs
call emcmake cmake .

REM Build the WebAssembly module
echo Building WebAssembly module...
call cmake --build .

cd ..

REM Check if the build was successful
if %ERRORLEVEL% NEQ 0 (
    echo Build failed.
    exit /b 1
)

echo Checking if WebAssembly files were generated...
if exist build-ecs\amamoto_ecs.js (
    echo WebAssembly files generated successfully!
    copy build-ecs\amamoto_ecs.js src\web\public\
    copy build-ecs\amamoto_ecs.wasm src\web\public\
    echo Files copied to src\web\public\
) else (
    echo Error: WebAssembly files were not generated.
    exit /b 1
)

echo ===================================================
echo ECS Build completed successfully!
echo ===================================================