#!/bin/bash
# Amamoto Setup Script for WebAssembly Compilation
# This script automates the process of setting up Emscripten and compiling the C++ code to WebAssembly

echo "==================================================="
echo "Amamoto WebAssembly Setup and Build Script"
echo "==================================================="
echo "This script will help you:"
echo "1. Install Emscripten SDK (if not already installed)"
echo "2. Configure the build environment"
echo "3. Compile the C++ code to WebAssembly"
echo "4. Integrate the WebAssembly module with the web frontend"
echo ""

# Check if git is installed
if ! command -v git &> /dev/null; then
    echo "Error: Git is required but not found."
    echo "Please install Git and try again."
    exit 1
fi

# Check if CMake is installed
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is required but not found."
    echo "Please install CMake and try again."
    exit 1
fi

echo "Checking for Emscripten SDK..."
# Check if emsdk directory already exists
if [ -d "emsdk" ]; then
    echo "Emscripten SDK directory found."
else
    echo "Cloning Emscripten SDK repository..."
    git clone https://github.com/emscripten-core/emsdk.git
    if [ $? -ne 0 ]; then
        echo "Error: Failed to clone Emscripten SDK repository."
        exit 1
    fi
fi

echo ""
echo "Setting up Emscripten SDK..."
cd emsdk
./emsdk install latest
if [ $? -ne 0 ]; then
    echo "Error: Failed to install Emscripten SDK."
    cd ..
    exit 1
fi

./emsdk activate latest
if [ $? -ne 0 ]; then
    echo "Error: Failed to activate Emscripten SDK."
    cd ..
    exit 1
fi

echo ""
echo "Setting up environment variables..."
source ./emsdk_env.sh

cd ..

echo ""
echo "Creating build directory..."
mkdir -p build

echo ""
echo "Configuring build with CMake and Emscripten..."
emcmake cmake -B build
if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed."
    exit 1
fi

echo ""
echo "Building WebAssembly module..."
cmake --build build
if [ $? -ne 0 ]; then
    echo "Error: Build failed."
    exit 1
fi

echo ""
echo "Checking if WebAssembly files were generated..."
if [ -f "build/amamoto_module.js" ]; then
    echo "WebAssembly files generated successfully!"
else
    echo "Error: WebAssembly files were not generated."
    exit 1
fi

echo ""
echo "==================================================="
echo "Build completed successfully!"
echo ""
echo "The WebAssembly module has been compiled and copied to:"
echo "  - src/web/public/amamoto_module.js"
echo "  - src/web/public/amamoto_module.wasm"
echo ""
echo "To use the WebAssembly module instead of the JavaScript implementation:"
echo "1. Edit src/web/public/simulation.js to import from wasm-integration.js"
echo "2. Restart your development server"
echo "==================================================="