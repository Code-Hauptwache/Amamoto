/**
 * Simulation Web Worker for Amamoto Traffic Simulation
 * This worker handles vehicle updates in a separate thread
 */

// Import shared worker utilities
import { WorkerUtils } from './worker-utils.js';

// Store vehicles assigned to this worker
let vehicleBatch = [];
let simulationBounds = { width: 800, height: 600 };
let keepInBounds = true;
let roadNetwork = null;

/**
 * Process a batch of vehicles and update their positions
 * @param {Array} vehicles - Array of vehicle data to update
 * @param {number} dt - Time step for the update (seconds)
 * @returns {Array} Updated vehicle data
 */
function processVehicleBatch(vehicles, dt) {
    return vehicles.map(vehicle => {
        // Skip inactive vehicles
        if (!vehicle.active) return vehicle;
        
        // Update position based on velocity
        vehicle.position.x += vehicle.velocity.x * dt;
        vehicle.position.y += vehicle.velocity.y * dt;
        
        // Apply bounds if needed
        if (keepInBounds) {
            // Check bounds for x
            if (vehicle.position.x < 0) {
                vehicle.position.x = 0;
                vehicle.velocity.x = -vehicle.velocity.x * 0.5; // Bounce with energy loss
            }
            else if (vehicle.position.x > simulationBounds.width) {
                vehicle.position.x = simulationBounds.width;
                vehicle.velocity.x = -vehicle.velocity.x * 0.5; // Bounce with energy loss
            }
            
            // Check bounds for y
            if (vehicle.position.y < 0) {
                vehicle.position.y = 0;
                vehicle.velocity.y = -vehicle.velocity.y * 0.5; // Bounce with energy loss
            }
            else if (vehicle.position.y > simulationBounds.height) {
                vehicle.position.y = simulationBounds.height;
                vehicle.velocity.y = -vehicle.velocity.y * 0.5; // Bounce with energy loss
            }
        }
        
        // Apply path following if this vehicle has a path and roadNetwork is available
        if (vehicle.path && vehicle.path.length > 0 && roadNetwork) {
            // Follow path logic here
            // ...
        }
        
        // Limit velocity to max speed
        const speed = Math.sqrt(
            vehicle.velocity.x * vehicle.velocity.x + 
            vehicle.velocity.y * vehicle.velocity.y
        );
        
        if (speed > vehicle.maxSpeed) {
            const scale = vehicle.maxSpeed / speed;
            vehicle.velocity.x *= scale;
            vehicle.velocity.y *= scale;
        }
        
        return vehicle;
    });
}

/**
 * Process collision detection between vehicles
 * @param {Array} vehicles - Array of vehicle data
 * @returns {Array} Updated vehicle data with collision responses applied
 */
function processCollisions(vehicles) {
    // Spatial grid for efficient collision detection
    const gridSize = 20; // Grid cell size in pixels
    const grid = new Map();
    
    // Insert vehicles into grid
    vehicles.forEach((vehicle, index) => {
        if (!vehicle.active) return;
        
        // Get grid cells that this vehicle occupies
        const cellX = Math.floor(vehicle.position.x / gridSize);
        const cellY = Math.floor(vehicle.position.y / gridSize);
        
        // Add vehicle to grid cell
        const cellKey = `${cellX},${cellY}`;
        if (!grid.has(cellKey)) {
            grid.set(cellKey, []);
        }
        grid.get(cellKey).push({ index, vehicle });
    });
    
    // Check for collisions within each grid cell and neighboring cells
    const processed = new Set();
    
    grid.forEach((cellVehicles, cellKey) => {
        // Check collisions within this cell
        for (let i = 0; i < cellVehicles.length; i++) {
            const v1 = cellVehicles[i];
            
            for (let j = i + 1; j < cellVehicles.length; j++) {
                const v2 = cellVehicles[j];
                
                // Skip if this pair has been processed
                const pairKey = v1.index < v2.index ? 
                    `${v1.index}-${v2.index}` : 
                    `${v2.index}-${v1.index}`;
                
                if (processed.has(pairKey)) continue;
                processed.add(pairKey);
                
                // Check for collision
                const dx = v2.vehicle.position.x - v1.vehicle.position.x;
                const dy = v2.vehicle.position.y - v1.vehicle.position.y;
                const distance = Math.sqrt(dx * dx + dy * dy);
                
                // Simple collision detection (assuming vehicles are circles)
                const minDistance = 5.0; // Minimum distance to consider a collision
                
                if (distance < minDistance) {
                    // Apply simple collision response
                    const nx = dx / distance;
                    const ny = dy / distance;
                    
                    // Separate vehicles
                    const separation = minDistance - distance;
                    
                    vehicles[v1.index].position.x -= nx * separation * 0.5;
                    vehicles[v1.index].position.y -= ny * separation * 0.5;
                    vehicles[v2.index].position.x += nx * separation * 0.5;
                    vehicles[v2.index].position.y += ny * separation * 0.5;
                    
                    // Exchange velocities (simplified)
                    const v1vx = vehicles[v1.index].velocity.x;
                    const v1vy = vehicles[v1.index].velocity.y;
                    
                    vehicles[v1.index].velocity.x = vehicles[v2.index].velocity.x * 0.9;
                    vehicles[v1.index].velocity.y = vehicles[v2.index].velocity.y * 0.9;
                    vehicles[v2.index].velocity.x = v1vx * 0.9;
                    vehicles[v2.index].velocity.y = v1vy * 0.9;
                }
            }
        }
    });
    
    return vehicles;
}

// Message handler for the worker
self.onmessage = function(e) {
    const message = e.data;
    
    switch (message.type) {
        case 'init':
            // Initialize the worker with simulation parameters
            simulationBounds = message.bounds;
            keepInBounds = message.keepInBounds;
            self.postMessage({ type: 'initialized' });
            break;
            
        case 'setRoadNetwork':
            // Set the road network data
            roadNetwork = message.roadNetwork;
            self.postMessage({ type: 'roadNetworkSet' });
            break;
            
        case 'updateVehicles':
            // Update a batch of vehicles
            const updatedVehicles = processVehicleBatch(message.vehicles, message.dt);
            
            // Process collisions if enabled
            const vehiclesWithCollisions = message.detectCollisions ? 
                processCollisions(updatedVehicles) : 
                updatedVehicles;
            
            // Send back the updated vehicles
            self.postMessage({
                type: 'vehiclesUpdated',
                vehicles: vehiclesWithCollisions,
                batchId: message.batchId
            });
            break;
            
        case 'setKeepInBounds':
            // Update the keep in bounds setting
            keepInBounds = message.value;
            self.postMessage({ type: 'settingUpdated', setting: 'keepInBounds', value: keepInBounds });
            break;
    }
};

// Notify that the worker is ready
self.postMessage({ type: 'ready' });