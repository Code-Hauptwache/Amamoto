# Amamoto - Technical Validation POC

This proof of concept (POC) is designed to validate the core technical approach for the Amamoto traffic simulation platform. It uses a JavaScript implementation to simulate what will eventually be the WebAssembly/C++ simulation engine.

## What This POC Validates

- **Module architecture**: Testing a basic entity system design for vehicles
- **Performance measurement**: Evaluating the fps and memory usage for a simple simulation
- **Visualization**: Testing the canvas-based rendering approach
- **Interface design**: Validating the API between the simulation engine and UI

## Prerequisites

- **Node.js** (version 14 or higher)
- A modern web browser

## Project Structure

```
amamoto/
├── src/
│   ├── simulation/         # C++ simulation code (for future WebAssembly build)
│   │   ├── simulation.h    # Core simulation classes
│   │   ├── simulation.cpp  # Implementation
│   │   └── wasm_module.cpp # Entry point for WebAssembly
│   └── web/
│       └── public/         # Web frontend
│           ├── index.html  # Main HTML page
│           ├── simulation.js  # JavaScript code to interact with simulation
│           └── js-simulation.js  # JavaScript implementation of simulation engine
├── CMakeLists.txt          # CMake build configuration (for future use)
├── package.json            # Node.js package configuration
└── POC_README.md           # This file
```

## Setup Instructions

1. Install project dependencies:
   ```
   npm install
   ```

## Running the POC

Start the development server:
```
npm start
```

This will serve the web application at http://localhost:8080 (or another port if 8080 is in use).

## What to Look For

Once the application is running, you should see:

- A canvas displaying animated circles representing vehicles
- Performance metrics showing:
  - Vehicle count
  - Frames per second (FPS)
  - Memory usage (simulated)

Try clicking "Add Vehicle" multiple times to increase the number of entities and observe how performance scales. The key metrics to evaluate:

1. How many vehicles can be simulated while maintaining 60fps?
2. How does the entity system design perform with increasing vehicle counts?
3. Is the architecture approach working as expected?

## Conclusions and Next Steps

Based on the performance of this POC, we can assess:

1. Whether the overall architecture works for our simulation needs
2. If the entity system design is efficient for our use case
3. How to structure the future WebAssembly implementation based on this design

If the core approach works well, the next steps would be:
1. Setting up the Emscripten SDK and implementing the C++ version
2. Implementing a road network representation
3. Adding realistic vehicle physics and collision detection
4. Creating a proper road editor UI
5. Exploring GPU acceleration options

After verifying the architecture works with this JavaScript implementation, we can proceed to implementing the C++ version that will be compiled to WebAssembly for better performance.