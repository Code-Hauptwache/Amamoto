// This file serves as the entry point for our WebAssembly module.
// It doesn't need much code since we're using Emscripten bindings in simulation.cpp.

#include "simulation.h"

// Optional: we could add any initialization code here if needed
int main() {
    // This main function is required by Emscripten but doesn't need to do anything
    // as our module is designed to be used through the bound functions
    return 0;
}