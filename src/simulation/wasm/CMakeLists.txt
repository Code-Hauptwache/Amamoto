cmake_minimum_required(VERSION 3.10)
project(amamoto_wasm_simulation)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check if we're using Emscripten for WebAssembly compilation
if(EMSCRIPTEN)
    # Set Emscripten-specific options
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    add_compile_options(-sWASM=1 -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_RUNTIME_METHODS=['UTF8ToString'] -fexceptions -frtti)
    set(LINK_FLAGS 
        "-sEXPORT_ES6=1 -sALLOW_MEMORY_GROWTH=1 -sMODULARIZE=1 -sEXPORT_NAME=AmattoSimulation -sNO_EXIT_RUNTIME=1 -sASSERTIONS=1"
    )
else()
    # Native build options (could be used for testing)
    add_compile_options(-Wall -Wextra)
endif()

# Create the main WebAssembly module directly (no library dependency)
if(EMSCRIPTEN)
    add_executable(amamoto_module
        SimpleTrafficSimulation.cpp
        wasm_bindings_simple.cpp
    )
    
    # Apply link flags for WebAssembly module
    set_target_properties(amamoto_module PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
    
    # Link with embind for JavaScript bindings
    target_link_libraries(amamoto_module "-lembind")
    
    # Copy to web directory
    add_custom_command(TARGET amamoto_module POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/amamoto_module.js
        ${CMAKE_BINARY_DIR}/amamoto_module.wasm
        ${CMAKE_CURRENT_SOURCE_DIR}/../../web/public/
    )
endif()