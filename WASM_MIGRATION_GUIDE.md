# WebAssembly Migration Guide for Amamoto

This guide will walk you through the process of transitioning from the JavaScript implementation to the WebAssembly implementation of the Amamoto traffic simulation.

## Prerequisites

Before proceeding, make sure you have:

1. Successfully built the WebAssembly module using either:
   - `setup-wasm-build.bat` (Windows)
   - `setup-wasm-build.sh` (Unix/macOS)

2. Confirmed that the following files exist in your `src/web/public` directory:
   - `amamoto_module.js` - JavaScript loader for the WebAssembly module
   - `amamoto_module.wasm` - The WebAssembly binary

## Migration Steps

### 1. Update simulation.js

The current simulation.js file loads a JavaScript implementation. We need to modify it to use the WebAssembly module instead.

**Find this code in simulation.js (line ~72):**

```javascript
// Import the JavaScript simulation (will be replaced with WebAssembly later)
const AmattoSimulation = await import('./js-simulation.js');
const module = await AmattoSimulation.default();

// Create a simulation instance
simulation = new module.Simulation();
wasmMemory = module.HEAP8.buffer;
```

**Replace it with:**

```javascript
// Import the WebAssembly simulation
import { loadWasmModule } from './wasm-integration.js';
const { simulation, memory } = await loadWasmModule();
wasmMemory = memory;
```

### 2. Update Traffic Integration

The traffic integration uses specific methods from the simulation. Ensure it's compatible with the WebAssembly implementation:

**Open traffic-integration.js and find method calls like:**

```javascript
// Add to simulation
const id = this.simulation.createVehicle(x, y, vx, vy);
```

These should continue to work with the WebAssembly implementation, as the wasm-integration.js file provides the same interface. If you encounter any issues, check that the method signatures match between the JavaScript and WebAssembly implementations.

### 3. Update Additional Parts of simulation.js

Some other parts of the code might need adjustments:

#### Vehicle Position and Velocity Retrieval

If you encounter issues with vehicle position or velocity retrieval, make sure your calls to these methods match the WebAssembly implementation:

```javascript
// Get vehicle position from WebAssembly
const position = simulation.getVehiclePosition(id);
```

#### Simulation Update

Update the simulation loop if necessary:

```javascript
// Update simulation with bounds
const bounds = {
    width: canvas.width,
    height: canvas.height
};
// For WebAssembly implementation
simulation.setKeepInBounds(keepInBoundsCb.checked);
simulation.update(deltaTime);
```

### 4. Test the Implementation

After making these changes:

1. Restart your development server
2. Navigate to the MVP page
3. Test both the road editor and simulation functionality
4. Verify that vehicles move correctly on the roads
5. Check performance metrics - the WebAssembly implementation should provide better performance

### 5. Debugging Common Issues

If you encounter issues with the WebAssembly implementation:

1. **Module Loading Errors**
   - Check the browser console for detailed error messages
   - Ensure the WebAssembly files were correctly copied to the public directory
   - Verify the paths in the import statements

2. **Method Not Found Errors**
   - Compare method signatures in the C++ implementation (wasm_bindings.cpp) with what's being called from JavaScript
   - Check for typos in method names

3. **Memory Access Issues**
   - If you see errors related to memory access, check that buffer transfers between JavaScript and WebAssembly are being handled correctly

4. **Performance Problems**
   - Profile the application using browser developer tools
   - Look for excessive memory allocations or unnecessary data transfers between JavaScript and WebAssembly

## Next Steps After Migration

Once the WebAssembly migration is complete, you can move forward with other improvements:

1. **Road Network Enhancements**
   - Add more sophisticated traffic rules
   - Implement traffic signals and signage

2. **GPU Acceleration**
   - Use WebGL for rendering and computation
   - Implement spatial partitioning for efficient collision detection

3. **Vehicle Behavior Refinement**
   - Add realistic car-following models
   - Implement lane-changing behavior