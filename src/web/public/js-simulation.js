// JavaScript implementation of the simulation module
// This mimics what our WebAssembly module would do, for testing without Emscripten

// Vector2D equivalent
class Vector2D {
    constructor(x = 0, y = 0) {
        this.x = x;
        this.y = y;
    }
}

// Vehicle entity
class Vehicle {
    constructor(id, x, y, vx, vy) {
        this.id = id;
        this.position = new Vector2D(x, y);
        this.velocity = new Vector2D(vx, vy);
    }
    
    getId() {
        return this.id;
    }
    
    getPosition() {
        return this.position;
    }
    
    getVelocity() {
        return this.velocity;
    }
    
    update(dt, bounds = null) {
        // Update position based on velocity
        this.position.x += this.velocity.x * dt;
        this.position.y += this.velocity.y * dt;
        
        // If bounds are provided, keep the vehicle within the bounds
        if (bounds) {
            // Bounce off edges with reversed velocity if hitting a boundary
            if (this.position.x < 0) {
                this.position.x = 0;
                this.velocity.x = -this.velocity.x;
            } else if (this.position.x > bounds.width) {
                this.position.x = bounds.width;
                this.velocity.x = -this.velocity.x;
            }
            
            if (this.position.y < 0) {
                this.position.y = 0;
                this.velocity.y = -this.velocity.y;
            } else if (this.position.y > bounds.height) {
                this.position.y = bounds.height;
                this.velocity.y = -this.velocity.y;
            }
        }
    }
}

// Simulation class
class Simulation {
    constructor() {
        this.vehicles = [];
    }
    
    createVehicle(x, y, vx, vy) {
        const id = this.vehicles.length;
        this.vehicles.push(new Vehicle(id, x, y, vx, vy));
        return id;
    }
    
    getVehicleCount() {
        return this.vehicles.length;
    }
    
    getVehiclePosition(id) {
        if (id >= 0 && id < this.vehicles.length) {
            return this.vehicles[id].getPosition();
        }
        return new Vector2D(0, 0);
    }
    
    getVehicleVelocity(id) {
        if (id >= 0 && id < this.vehicles.length) {
            return this.vehicles[id].getVelocity();
        }
        return new Vector2D(0, 0);
    }
    
    updateVehicle(id, pos, vel) {
        if (id >= 0 && id < this.vehicles.length) {
            this.vehicles[id].position.x = pos.x;
            this.vehicles[id].position.y = pos.y;
            this.vehicles[id].velocity.x = vel.x;
            this.vehicles[id].velocity.y = vel.y;
            return true;
        }
        return false;
    }
    
    update(dt, bounds = null) {
        for (const vehicle of this.vehicles) {
            vehicle.update(dt, bounds);
        }
    }
    
    clear() {
        this.vehicles = [];
    }
}

// Export a factory function that mimics how the WebAssembly module would be loaded
export default function() {
    // Create a dynamic buffer that grows with the number of vehicles
    // Start with 1MB (1024 * 1024 bytes)
    let simulatedMemory = new ArrayBuffer(1024 * 1024);
    
    // Return a mock module object with methods to track memory
    return {
        Simulation: Simulation,
        // Mimicking WASM memory with dynamic sizing based on vehicle count
        HEAP8: {
            buffer: simulatedMemory,
            // Update the simulated memory size based on vehicle count
            // This makes memory usage more realistic for testing
            updateMemorySize: function(vehicleCount) {
                // Simulate memory growth: base 1MB + 2KB per vehicle
                const newSize = 1024 * 1024 + vehicleCount * 2 * 1024;
                if (newSize > simulatedMemory.byteLength) {
                    simulatedMemory = new ArrayBuffer(newSize);
                    this.buffer = simulatedMemory;
                }
                return this.buffer;
            }
        }
    };
}