#pragma once

#include "core/Entity.h"
#include "core/Components.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace amamoto {

class TrafficSimulation {
public:
    TrafficSimulation();
    ~TrafficSimulation();
    
    // Initialize the simulation with specific dimensions
    void initialize(float width, float height);
    
    // Create a vehicle and return its ID
    int createVehicle(float x, float y, float vx, float vy);
    
    // Get vehicle count
    int getVehicleCount() const;
    
    // Get vehicle position by ID
    Vector2D getVehiclePosition(int id) const;
    
    // Get vehicle velocity by ID
    Vector2D getVehicleVelocity(int id) const;
    
    // Update simulation by the given time step
    void update(float dt);
    
    // Set whether to keep vehicles in bounds
    void setKeepInBounds(bool keepInBounds);
    
    // Check if vehicles are kept in bounds
    bool getKeepInBounds() const;
    
    // Clear all vehicles
    void clear();
    
    // Memory management
    void reserveVehicles(int count);
    
private:
    // Entity-Component-System world
    std::unique_ptr<World> m_world;
    
    // Simulation bounds
    float m_width;
    float m_height;
    bool m_keepInBounds;
    
    // Entity ID to the simulation's internal ID mapping
    std::unordered_map<EntityID, int> m_entityToSimID;
    std::unordered_map<int, EntityID> m_simIDToEntity;
    int m_nextSimID;
    
    // Flag to track if initialized
    bool m_initialized;
    
    // Update vehicle transforms
    void updateVehicleTransforms(float dt);
    
    // Check and handle bounds
    void handleBounds();
};

#ifdef __EMSCRIPTEN__
// Declare extern C functions for Emscripten to bind
extern "C" {
    TrafficSimulation* TrafficSimulation_Create();
    void TrafficSimulation_Destroy(TrafficSimulation* simulation);
    void TrafficSimulation_Initialize(TrafficSimulation* simulation, float width, float height);
    int TrafficSimulation_CreateVehicle(TrafficSimulation* simulation, float x, float y, float vx, float vy);
    int TrafficSimulation_GetVehicleCount(TrafficSimulation* simulation);
    void TrafficSimulation_GetVehiclePosition(TrafficSimulation* simulation, int id, float* x, float* y);
    void TrafficSimulation_GetVehicleVelocity(TrafficSimulation* simulation, int id, float* vx, float* vy);
    void TrafficSimulation_Update(TrafficSimulation* simulation, float dt);
    void TrafficSimulation_SetKeepInBounds(TrafficSimulation* simulation, bool keepInBounds);
    bool TrafficSimulation_GetKeepInBounds(TrafficSimulation* simulation);
    void TrafficSimulation_Clear(TrafficSimulation* simulation);
    void TrafficSimulation_ReserveVehicles(TrafficSimulation* simulation, int count);
}
#endif

} // namespace amamoto