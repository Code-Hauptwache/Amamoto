#!/bin/bash
# Amamoto WebAssembly Build Script for Simplified Implementation
# This script builds only the simplified traffic simulation without template issues

echo "==================================================="
echo "Building Simplified WebAssembly Module"
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
echo "Creating build directory for simplified module..."
mkdir -p build-simple

# Configure and build
echo "Configuring build with CMake and Emscripten..."
emcmake cmake -B build-simple -S src/simulation/wasm

echo "Building WebAssembly module..."
cmake --build build-simple

# Check if the build was successful
if [ $? -ne 0 ]; then
    echo "Build failed."
    exit 1
fi

# Copy output files to web directory
echo "Copying files to web directory..."
if [ -f "build-simple/amamoto_module.js" ]; then
    cp build-simple/amamoto_module.js src/web/public/
    cp build-simple/amamoto_module.wasm src/web/public/
else
    echo "Warning: WebAssembly files were not found in expected location"
fi

echo "==================================================="
echo "Build completed!"
echo "==================================================="
echo "The simplified WebAssembly module has been compiled and copied to:"
echo "  - src/web/public/amamoto_module.js"
echo "  - src/web/public/amamoto_module.wasm"
echo ""
echo "To use the WebAssembly module, update simulation.js as described in WASM_MIGRATION_GUIDE.md"
echo "==================================================="