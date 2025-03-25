#!/bin/bash
# Amamoto WebAssembly ECS Build Script
# This script builds the ECS-based traffic simulation

echo "==================================================="
echo "Building ECS WebAssembly Module"
echo "==================================================="

# Check if Emscripten SDK is set up
if [ ! -d "emsdk" ]; then
    echo "Error: Emscripten SDK is not set up."
    echo "Please run setup-wasm-build.sh first to install the Emscripten SDK."
    exit 1
fi

# Set up Emscripten environment
echo "Setting up Emscripten environment..."
cd emsdk
source ./emsdk_env.sh
cd ..

# Create build directory
echo "Creating build directory for ECS module..."
mkdir -p build-ecs

# Create standalone CMakeLists.txt for the ECS module
echo "Creating CMakeLists.txt for ECS module..."

cat > build-ecs/CMakeLists.txt << EOL
cmake_minimum_required(VERSION 3.10)
project(amamoto_ecs_simulation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    add_compile_options(-sWASM=1 -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_RUNTIME_METHODS=['UTF8ToString'] -fexceptions -frtti)
    set(LINK_FLAGS "-sEXPORT_ES6=1 -sALLOW_MEMORY_GROWTH=1 -sMODULARIZE=1 -sEXPORT_NAME=AmattoECSSimulation -sNO_EXIT_RUNTIME=1 -sASSERTIONS=1")
endif()

add_executable(amamoto_ecs
    $(pwd)/src/simulation/wasm/WasmECS.cpp
    $(pwd)/src/simulation/wasm/WasmRoadNetwork.cpp
    $(pwd)/src/simulation/wasm/WasmTrafficSimulation.cpp
    $(pwd)/src/simulation/wasm/wasm_bindings_ecs.cpp
)

target_include_directories(amamoto_ecs PUBLIC $(pwd)/src)

if(EMSCRIPTEN)
    set_target_properties(amamoto_ecs PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
    target_link_libraries(amamoto_ecs "-lembind")
endif()
EOL

# Run CMake to configure the build
echo "Configuring build with CMake and Emscripten..."
cd build-ecs
emcmake cmake .

# Build the WebAssembly module
echo "Building WebAssembly module..."
cmake --build .

cd ..

# Check if the build was successful
if [ $? -ne 0 ]; then
    echo "Build failed."
    exit 1
fi

echo "Checking if WebAssembly files were generated..."
if [ -f "build-ecs/amamoto_ecs.js" ]; then
    echo "WebAssembly files generated successfully!"
    cp build-ecs/amamoto_ecs.js src/web/public/
    cp build-ecs/amamoto_ecs.wasm src/web/public/
    echo "Files copied to src/web/public/"
else
    echo "Error: WebAssembly files were not generated."
    exit 1
fi

echo "==================================================="
echo "ECS Build completed successfully!"
echo "==================================================="