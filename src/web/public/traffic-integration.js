// Traffic integration module to connect the road network with vehicle simulation
import { Vector2D, RoadNetwork } from './road-network.js';

class TrafficIntegration {
    constructor(roadNetwork, simulation) {
        this.roadNetwork = roadNetwork;
        this.simulation = simulation;
        this.vehiclePathMap = new Map(); // Maps vehicle IDs to their paths
        this.vehicleProgressMap = new Map(); // Maps vehicle IDs to their progress along the path
    }
    
    // Add a vehicle to a specific road
    addVehicleToRoad(roadId, laneId = 0, progress = 0) {
        // Get the road segment
        const road = this.roadNetwork.roadSegments.get(roadId);
        if (!road) return -1;
        
        // Get position along the road at the specified progress
        const position = road.getPointAtDistance(progress * road.length);
        const direction = road.getDirectionAtDistance(progress * road.length);
        
        // Calculate velocity based on direction
        const speed = 30 + Math.random() * 20; // Random speed between 30-50 units/sec
        const velocity = {
            x: direction.x * speed,
            y: direction.y * speed
        };
        
        // Create the vehicle in the simulation
        const vehicleId = this.simulation.createVehicle(position.x, position.y, velocity.x, velocity.y);
        
        // Store the path information for this vehicle
        this.vehiclePathMap.set(vehicleId, [{ roadId, laneId }]);
        this.vehicleProgressMap.set(vehicleId, progress);
        
        return vehicleId;
    }
    
    // Add a vehicle that follows a path
    addVehicleWithPath(path, initialProgress = 0) {
        if (!path || path.length === 0) return -1;
        
        // Get the first road segment in the path
        const firstRoadInfo = path[0];
        const road = this.roadNetwork.roadSegments.get(firstRoadInfo.roadId);
        if (!road) return -1;
        
        // Get position along the road at the specified progress
        const position = road.getPointAtDistance(initialProgress * road.length);
        const direction = road.getDirectionAtDistance(initialProgress * road.length);
        
        // Calculate velocity based on direction
        const speed = 30 + Math.random() * 20; // Random speed between 30-50 units/sec
        const velocity = {
            x: direction.x * speed,
            y: direction.y * speed
        };
        
        // Create the vehicle in the simulation
        const vehicleId = this.simulation.createVehicle(position.x, position.y, velocity.x, velocity.y);
        
        // Store the path information for this vehicle
        this.vehiclePathMap.set(vehicleId, [...path]); // Clone the path
        this.vehicleProgressMap.set(vehicleId, initialProgress);
        
        return vehicleId;
    }
    
    // Add a vehicle with a random path
    addRandomVehicleOnNetwork() {
        // Get all road segments
        const roads = Array.from(this.roadNetwork.roadSegments.values());
        if (roads.length === 0) return -1;
        
        // Pick a random starting road
        const startRoad = roads[Math.floor(Math.random() * roads.length)];
        
        // Pick a random ending road (different from starting road)
        let endRoad;
        if (roads.length > 1) {
            do {
                endRoad = roads[Math.floor(Math.random() * roads.length)];
            } while (endRoad.id === startRoad.id);
        } else {
            // If there's only one road, use it for both start and end
            endRoad = startRoad;
        }
        
        // Get random positions on the start and end roads
        const startPos = startRoad.getPointAtDistance(Math.random() * startRoad.length);
        const endPos = endRoad.getPointAtDistance(Math.random() * endRoad.length);
        
        // Find a path between these two positions
        const path = this.roadNetwork.findPath(startPos.x, startPos.y, endPos.x, endPos.y);
        
        // Add a vehicle following this path
        return this.addVehicleWithPath(path);
    }
    
    // Add multiple random vehicles
    addRandomVehicles(count) {
        const vehicles = [];
        for (let i = 0; i < count; i++) {
            const id = this.addRandomVehicleOnNetwork();
            if (id >= 0) {
                vehicles.push(id);
            }
        }
        return vehicles;
    }
    
    // Function to update the simulation
    updateVehicle(id, position, velocity) {
        if (!this.simulation) return;
        
        // Update the vehicle in the simulation
        return this.simulation.updateVehicle(id, position, velocity);
    }
    
    // Update vehicle positions based on their paths
    updateVehicles(dt) {
        // Get vehicle count from simulation
        const vehicleCount = this.simulation.getVehicleCount();
        
        // Loop through all vehicles
        for (let i = 0; i < vehicleCount; i++) {
            // Check if this vehicle has a path
            if (!this.vehiclePathMap.has(i)) continue;
            
            const path = this.vehiclePathMap.get(i);
            let progress = this.vehicleProgressMap.get(i);
            
            // Get current road segment
            const currentRoadInfo = path[0];
            const road = this.roadNetwork.roadSegments.get(currentRoadInfo.roadId);
            
            if (!road) {
                // Road no longer exists, remove path
                this.vehiclePathMap.delete(i);
                this.vehicleProgressMap.delete(i);
                continue;
            }
            
            // Use a fixed speed for vehicles on roads (can be adjusted)
            const speed = 40; // units per second
            
            // Calculate distance traveled along road
            const distanceTraveled = speed * dt;
            
            // Update progress along the road
            progress += distanceTraveled / road.length;
            
            // Check if we've reached the end of this road segment
            if (progress >= 1.0) {
                // Move to next road segment in path
                path.shift();
                
                // If there are no more road segments, loop back to start
                if (path.length === 0) {
                    // For simplicity in MVP, we'll just pick a new random path
                    if (this.roadNetwork.roadSegments.size > 0) {
                        const roads = Array.from(this.roadNetwork.roadSegments.values());
                        const newRoad = roads[Math.floor(Math.random() * roads.length)];
                        path.push({ roadId: newRoad.id, laneId: 0 });
                        progress = 0;
                    } else {
                        // No roads available, remove the vehicle from tracking
                        this.vehiclePathMap.delete(i);
                        this.vehicleProgressMap.delete(i);
                        continue;
                    }
                } else {
                    // Reset progress for the new road segment
                    progress = 0;
                }
            }
            
            // Update the stored progress
            this.vehicleProgressMap.set(i, progress);
            
            // Calculate new position and direction along the road
            const newPosition = road.getPointAtDistance(progress * road.length);
            const direction = road.getDirectionAtDistance(progress * road.length);
            
            // Calculate new velocity
            const newVelocity = {
                x: direction.x * speed,
                y: direction.y * speed
            };
            
            // Update vehicle in simulation
            this.updateVehicle(i, newPosition, newVelocity);
        }
    }
    
    // Get the network state
    getNetworkState() {
        return {
            roadCount: this.roadNetwork.roadSegments.size,
            intersectionCount: this.roadNetwork.intersections.size,
            vehiclesOnNetwork: this.vehiclePathMap.size
        };
    }
}

export { TrafficIntegration };