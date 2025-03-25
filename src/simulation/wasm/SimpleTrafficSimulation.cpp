#include "SimpleTrafficSimulation.h"
#include <algorithm>

namespace amamoto {

WasmTrafficSimulation::WasmTrafficSimulation()
    : m_width(800.0f)
    , m_height(600.0f)
    , m_keepInBounds(true)
    , m_nextId(0)
{
    // Reserve space for 100 vehicles by default
    m_vehicles.reserve(100);
}

WasmTrafficSimulation::~WasmTrafficSimulation() {
    // Clear vehicles
    m_vehicles.clear();
}

void WasmTrafficSimulation::initialize(float width, float height) {
    m_width = width;
    m_height = height;
}

int WasmTrafficSimulation::createVehicle(float x, float y, float vx, float vy) {
    // Find an inactive vehicle or create a new one
    int id = -1;
    for (size_t i = 0; i < m_vehicles.size(); ++i) {
        if (!m_vehicles[i].active) {
            id = static_cast<int>(i);
            break;
        }
    }
    
    if (id == -1) {
        // No inactive vehicle found, create a new one
        id = m_nextId++;
        m_vehicles.resize(m_nextId);
    }
    
    // Initialize the vehicle
    WasmVehicle& vehicle = m_vehicles[id];
    vehicle.position = WasmVector2D(x, y);
    vehicle.velocity = WasmVector2D(vx, vy);
    vehicle.maxSpeed = 100.0f;
    vehicle.active = true;
    
    return id;
}

int WasmTrafficSimulation::getVehicleCount() const {
    int count = 0;
    for (const auto& vehicle : m_vehicles) {
        if (vehicle.active) {
            count++;
        }
    }
    return count;
}

WasmVector2D WasmTrafficSimulation::getVehiclePosition(int id) const {
    if (id >= 0 && id < static_cast<int>(m_vehicles.size()) && m_vehicles[id].active) {
        return m_vehicles[id].position;
    }
    return WasmVector2D(0, 0);
}

WasmVector2D WasmTrafficSimulation::getVehicleVelocity(int id) const {
    if (id >= 0 && id < static_cast<int>(m_vehicles.size()) && m_vehicles[id].active) {
        return m_vehicles[id].velocity;
    }
    return WasmVector2D(0, 0);
}

void WasmTrafficSimulation::update(float dt) {
    // Update all active vehicles
    for (auto& vehicle : m_vehicles) {
        if (!vehicle.active) continue;
        
        // Update position based on velocity
        vehicle.position = vehicle.position + vehicle.velocity * dt;
        
        // Apply bounds if needed
        if (m_keepInBounds) {
            // Check bounds for x
            if (vehicle.position.x < 0) {
                vehicle.position.x = 0;
                vehicle.velocity.x = -vehicle.velocity.x * 0.5f; // Bounce with energy loss
            }
            else if (vehicle.position.x > m_width) {
                vehicle.position.x = m_width;
                vehicle.velocity.x = -vehicle.velocity.x * 0.5f; // Bounce with energy loss
            }
            
            // Check bounds for y
            if (vehicle.position.y < 0) {
                vehicle.position.y = 0;
                vehicle.velocity.y = -vehicle.velocity.y * 0.5f; // Bounce with energy loss
            }
            else if (vehicle.position.y > m_height) {
                vehicle.position.y = m_height;
                vehicle.velocity.y = -vehicle.velocity.y * 0.5f; // Bounce with energy loss
            }
        }
        
        // Limit velocity to max speed
        float speed = vehicle.velocity.length();
        if (speed > vehicle.maxSpeed) {
            vehicle.velocity = vehicle.velocity.normalize() * vehicle.maxSpeed;
        }
    }
}

void WasmTrafficSimulation::setKeepInBounds(bool keepInBounds) {
    m_keepInBounds = keepInBounds;
}

bool WasmTrafficSimulation::getKeepInBounds() const {
    return m_keepInBounds;
}

void WasmTrafficSimulation::clear() {
    // Mark all vehicles as inactive
    for (auto& vehicle : m_vehicles) {
        vehicle.active = false;
    }
    m_nextId = 0;
}

void WasmTrafficSimulation::reserveVehicles(int count) {
    m_vehicles.reserve(count);
}

} // namespace amamoto