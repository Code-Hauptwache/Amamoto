#pragma once

#include "WasmECS.h"
#include "WasmComponents.h"
#include "WasmRoadNetwork.h"
#include <string>
#include <functional>

namespace amamoto {

// Main traffic simulation class using the WebAssembly ECS
class WasmTrafficSimulation {
public:
    WasmTrafficSimulation();
    ~WasmTrafficSimulation();
    
    // Initialize the simulation with specific dimensions
    void initialize(float width, float height);
    
    // Create a vehicle and return its ID
    WasmEntityID createVehicle(float x, float y, float vx, float vy);
    
    // Get vehicle count
    int getVehicleCount() const;
    
    // Get vehicle position by ID
    WasmVector2D getVehiclePosition(WasmEntityID id) const;
    
    // Get vehicle velocity by ID
    WasmVector2D getVehicleVelocity(WasmEntityID id) const;
    
    // Update simulation by the given time step
    void update(float dt);
    
    // Set whether to keep vehicles in bounds
    void setKeepInBounds(bool keepInBounds);
    
    // Check if vehicles are kept in bounds
    bool getKeepInBounds() const;
    
    // Set the road network for this simulation
    void setRoadNetwork(WasmRoadNetwork* roadNetwork);
    
    // Get the road network
    WasmRoadNetwork* getRoadNetwork() const;
    
    // Create a path for a vehicle between two points
    bool createPath(WasmEntityID vehicleId, float startX, float startY, float endX, float endY);
    
    // Clear all vehicles
    void clear();
    
    // Memory management
    void reserveVehicles(int count);
    
    // Get the ECS world
    WasmWorld& getWorld() { return m_world; }
    const WasmWorld& getWorld() const { return m_world; }
    
private:
    // ECS world
    WasmWorld m_world;
    
    // Road network
    WasmRoadNetwork* m_roadNetwork;
    
    // Simulation parameters
    float m_width;
    float m_height;
    bool m_keepInBounds;
    
    // System implementations
    void setupSystems();
    
    // Movement system - updates positions based on velocities
    static void movementSystem(WasmWorld& world, float dt);
    
    // Bounds system - keeps vehicles within bounds
    static void boundsSystem(WasmWorld& world, float dt);
    
    // Path following system - makes vehicles follow paths
    static void pathFollowingSystem(WasmWorld& world, float dt, WasmRoadNetwork* roadNetwork);
    
    // Collision system - detects and resolves collisions
    static void collisionSystem(WasmWorld& world, float dt);
    
    // Traffic signal system - updates traffic signals
    static void trafficSignalSystem(WasmWorld& world, float dt);
    
    // Create path between two points on the road network
    std::vector<std::pair<WasmRoadSegmentID, WasmLaneID>> findPath(
        float startX, float startY, float endX, float endY);
};

// Define explicit system wrapper
class WasmSystemWrapper {
public:
    // Wrapper for the path following system that captures the road network pointer
    static std::function<void(WasmWorld&, float)> createPathFollowingSystem(WasmRoadNetwork* roadNetwork) {
        return [roadNetwork](WasmWorld& world, float dt) {
            WasmTrafficSimulation::pathFollowingSystem(world, dt, roadNetwork);
        };
    }
};

} // namespace amamoto