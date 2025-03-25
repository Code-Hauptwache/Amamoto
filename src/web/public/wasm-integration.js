/**
 * This file will handle the integration between the WebAssembly module and the frontend.
 * Once you have Emscripten installed and compile the C++ code, this will replace the JavaScript simulation.
 */

// Function to load the WebAssembly module
async function loadWasmModule() {
  try {
    // Import the WebAssembly module
    const AmattoSimulation = await import('./amamoto_module.js');
    const module = await AmattoSimulation.default();
    
    // Create simulation instance
    const simulation = new module.TrafficSimulation();
    
    // Initialize it with the canvas dimensions
    const canvas = document.getElementById('simulation-canvas');
    simulation.initialize(canvas.width, canvas.height);
    
    return {
      simulation,
      memory: module.HEAP8.buffer
    };
  } catch (error) {
    console.error('Failed to load WebAssembly module:', error);
    throw error;
  }
}

// Function to create a vehicle in the WebAssembly simulation
function createVehicle(simulation, x, y, vx, vy) {
  return simulation.createVehicle(x, y, vx, vy);
}

// Function to get the vehicle count
function getVehicleCount(simulation) {
  return simulation.getVehicleCount();
}

// Function to get a vehicle's position
function getVehiclePosition(simulation, id) {
  return simulation.getVehiclePosition(id);
}

// Function to update the simulation
function updateSimulation(simulation, dt, keepInBounds, canvasWidth, canvasHeight) {
  // Set whether to keep vehicles in bounds
  simulation.setKeepInBounds(keepInBounds);
  
  // Update the simulation
  simulation.update(dt);
}

// Function to clear the simulation
function clearSimulation(simulation) {
  simulation.clear();
}

// Function to add multiple vehicles
function addMultipleVehicles(simulation, count, canvasWidth, canvasHeight) {
  // Pre-allocate memory for better performance
  simulation.reserveVehicles(simulation.getVehicleCount() + count);
  
  // Add the vehicles
  for (let i = 0; i < count; i++) {
    // Random position within canvas
    const x = Math.random() * canvasWidth;
    const y = Math.random() * canvasHeight;
    
    // Random velocity
    const vx = (Math.random() - 0.5) * 100;
    const vy = (Math.random() - 0.5) * 100;
    
    createVehicle(simulation, x, y, vx, vy);
  }
}

export {
  loadWasmModule,
  createVehicle,
  getVehicleCount,
  getVehiclePosition,
  updateSimulation,
  clearSimulation,
  addMultipleVehicles
};