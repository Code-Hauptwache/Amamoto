/**
 * This file provides integration for the WebAssembly ECS-based traffic simulation.
 * It implements the same interface as the wasm-integration.js file but uses
 * the new ECS-based WebAssembly module instead.
 */

// Function to load the WebAssembly ECS module
async function loadWasmECSModule() {
  try {
    // Import the WebAssembly module
    const AmattoECSSimulation = await import('./amamoto_ecs.js');
    const module = await AmattoECSSimulation.default();
    
    // Create simulation instance
    const simulation = new module.TrafficSimulation();
    
    // Create road network
    const roadNetwork = new module.RoadNetwork();
    
    // Initialize it with the canvas dimensions
    const canvas = document.getElementById('simulation-canvas');
    simulation.initialize(canvas.width, canvas.height);
    
    console.log('ECS-based WebAssembly module loaded successfully');
    
    return {
      simulation,
      roadNetwork,
      memory: module.HEAP8.buffer
    };
  } catch (error) {
    console.error('Failed to load ECS-based WebAssembly module:', error);
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

// Function to get a vehicle's velocity
function getVehicleVelocity(simulation, id) {
  return simulation.getVehicleVelocity(id);
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

// Function to create a road segment
function createRoadSegment(roadNetwork, startX, startY, endX, endY) {
  return roadNetwork.createRoadSegment(startX, startY, endX, endY);
}

// Function to create an intersection
function createIntersection(roadNetwork, x, y) {
  return roadNetwork.createIntersection(x, y);
}

// Function to connect road segments with an intersection
function connectRoads(roadNetwork, road1Id, road1End, road2Id, road2End) {
  return roadNetwork.connectWithIntersection(road1Id, road1End, road2Id, road2End);
}

// Function to create a path for a vehicle
function createPath(simulation, vehicleId, startX, startY, endX, endY) {
  return simulation.createPath(vehicleId, startX, startY, endX, endY);
}

// Export all functions
export {
  loadWasmECSModule,
  createVehicle,
  getVehicleCount,
  getVehiclePosition,
  getVehicleVelocity,
  updateSimulation,
  clearSimulation,
  addMultipleVehicles,
  createRoadSegment,
  createIntersection,
  connectRoads,
  createPath
};