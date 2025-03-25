#pragma once

#include <vector>
#include <algorithm>
#include <cmath>

namespace amamoto {

// Simple 2D vector for WASM version
struct WasmVector2D {
    float x;
    float y;

    WasmVector2D() : x(0), y(0) {}
    WasmVector2D(float x_, float y_) : x(x_), y(y_) {}

    float length() const {
        return std::sqrt(x * x + y * y);
    }

    WasmVector2D normalize() const {
        float len = length();
        if (len < 0.0001f) return WasmVector2D(0, 0);
        return WasmVector2D(x / len, y / len);
    }

    WasmVector2D operator+(const WasmVector2D& other) const {
        return WasmVector2D(x + other.x, y + other.y);
    }

    WasmVector2D operator-(const WasmVector2D& other) const {
        return WasmVector2D(x - other.x, y - other.y);
    }

    WasmVector2D operator*(float scalar) const {
        return WasmVector2D(x * scalar, y * scalar);
    }
};

// Simple vehicle for WASM version
struct WasmVehicle {
    WasmVector2D position;
    WasmVector2D velocity;
    float maxSpeed;
    bool active;

    WasmVehicle() : position(0, 0), velocity(0, 0), maxSpeed(100.0f), active(false) {}
};

// Simplified Traffic Simulation for WebAssembly
class WasmTrafficSimulation {
public:
    WasmTrafficSimulation();
    ~WasmTrafficSimulation();
    
    // Initialize the simulation with specific dimensions
    void initialize(float width, float height);
    
    // Create a vehicle and return its ID
    int createVehicle(float x, float y, float vx, float vy);
    
    // Get vehicle count
    int getVehicleCount() const;
    
    // Get vehicle position by ID
    WasmVector2D getVehiclePosition(int id) const;
    
    // Get vehicle velocity by ID
    WasmVector2D getVehicleVelocity(int id) const;
    
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
    std::vector<WasmVehicle> m_vehicles;
    float m_width;
    float m_height;
    bool m_keepInBounds;
    int m_nextId;
};

} // namespace amamoto