#pragma once

#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#endif

namespace amamoto {

// Simple 2D vector for positions/velocities
struct Vector2D {
    float x = 0.0f;
    float y = 0.0f;

    Vector2D() = default;
    Vector2D(float x_, float y_) : x(x_), y(y_) {}
};

// Simple vehicle entity
class Vehicle {
public:
    Vehicle(int id, float x, float y, float vx, float vy);
    
    int getId() const { return m_id; }
    Vector2D getPosition() const { return m_position; }
    Vector2D getVelocity() const { return m_velocity; }
    
    void update(float dt);
    
private:
    int m_id;
    Vector2D m_position;
    Vector2D m_velocity;
};

// Simple simulation class
class Simulation {
public:
    Simulation();
    ~Simulation();
    
    // Create a vehicle and return its ID
    int createVehicle(float x, float y, float vx, float vy);
    
    // Get total vehicle count
    int getVehicleCount() const;
    
    // Get vehicle position by ID
    Vector2D getVehiclePosition(int id) const;
    
    // Update simulation by the given time step
    void update(float dt);
    
    // Clear all vehicles
    void clear();
    
private:
    std::vector<Vehicle> m_vehicles;
};

} // namespace amamoto