# Amamoto C++ Simulation Engine

This directory contains the C++ implementation of the Amamoto traffic simulation engine, designed to be compiled to WebAssembly for high-performance simulation in the browser.

## Directory Structure

```
src/simulation/
├── core/                # Core ECS architecture
│   ├── Entity.h         # Entity Component System core
│   ├── Entity.cpp       # Implementation of ECS
│   └── Components.h     # Component definitions
├── TrafficSimulation.h  # Main simulation interface
├── TrafficSimulation.cpp # Implementation of simulation
├── wasm_bindings.cpp    # Emscripten bindings for WebAssembly
└── README.md            # This file
```

## Architecture Overview

The simulation is built on an Entity Component System (ECS) architecture:

1. **Entities** are lightweight handles representing vehicles and other objects
2. **Components** store data like position, velocity, and vehicle properties
3. **Systems** process entities with specific components to update the simulation

This architecture provides:
- Cache-friendly memory layout for better performance
- Flexible composition for adding new features
- Clear separation of data and logic

## Compiling to WebAssembly

### Prerequisites

1. Install the Emscripten SDK:
   ```
   # Clone the emsdk repo
   git clone https://github.com/emscripten-core/emsdk.git
   
   # Enter the directory
   cd emsdk
   
   # Download and install the latest SDK
   ./emsdk install latest
   
   # Activate the latest SDK
   ./emsdk activate latest
   
   # Set up environment variables
   source ./emsdk_env.sh  # On Windows, use: emsdk_env.bat
   ```

2. Make sure CMake is installed (version 3.10 or higher)

### Build Process

1. From the project root, run:
   ```
   # Configure CMake with Emscripten
   emcmake cmake -B build
   
   # Build the project
   cmake --build build
   ```

2. The compiled WebAssembly module will be output to:
   - `build/amamoto_module.js` - JavaScript loader
   - `build/amamoto_module.wasm` - WebAssembly binary

3. The build process will automatically copy these files to `src/web/public/` for use in the web frontend.

## Integration with Web Frontend

The `src/web/public/wasm-integration.js` file provides the interface between the WebAssembly module and the frontend. Once compiled, switch from using the JavaScript simulation to the WebAssembly implementation by:

1. In `simulation.js`, change:
   ```javascript
   const AmattoSimulation = await import('./js-simulation.js');
   ```
   to:
   ```javascript
   import { loadWasmModule } from './wasm-integration.js';
   const { simulation, memory } = await loadWasmModule();
   ```

2. Update the rest of the frontend code to use the imported functions from `wasm-integration.js`.

## Performance Considerations

The C++ implementation uses several optimization techniques:
- Memory pooling to minimize allocations
- Cache-friendly data structures
- Minimized data transfer between JavaScript and WebAssembly
- Explicit memory management

## Next Steps for Development

1. **Road Network Implementation**:
   - Add graph-based road network representation
   - Implement lane and intersection logic
   - Add pathfinding for vehicles

2. **Physics Improvements**:
   - Add realistic vehicle dynamics
   - Implement proper collision detection
   - Add traffic rules enforcement

3. **GPU Acceleration**:
   - Implement WebGL-based acceleration
   - Add compute shaders for position updates
   - Optimize rendering pipeline

4. **Distributed Computing**:
   - Implement work division for large simulations
   - Add synchronization between workers
   - Create verification mechanisms