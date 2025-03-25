#include "simulation.h"

namespace amamoto {

Vehicle::Vehicle(int id, float x, float y, float vx, float vy)
    : m_id(id), m_position({x, y}), m_velocity({vx, vy}) {}

void Vehicle::update(float dt) {
    m_position.x += m_velocity.x * dt;
    m_position.y += m_velocity.y * dt;
}

Simulation::Simulation() {}

Simulation::~Simulation() {
    clear();
}

int Simulation::createVehicle(float x, float y, float vx, float vy) {
    // Simple ID generation by using the vector index
    int id = static_cast<int>(m_vehicles.size());
    m_vehicles.emplace_back(id, x, y, vx, vy);
    return id;
}

int Simulation::getVehicleCount() const {
    return static_cast<int>(m_vehicles.size());
}

Vector2D Simulation::getVehiclePosition(int id) const {
    // Basic validation to avoid out-of-bounds access
    if (id >= 0 && id < static_cast<int>(m_vehicles.size())) {
        return m_vehicles[id].getPosition();
    }
    return {0.0f, 0.0f}; // Default position if ID is invalid
}

void Simulation::update(float dt) {
    for (auto& vehicle : m_vehicles) {
        vehicle.update(dt);
    }
}

void Simulation::clear() {
    m_vehicles.clear();
}

} // namespace amamoto

#ifdef __EMSCRIPTEN__
// Binding code for WebAssembly exports
#include <emscripten/bind.h>

using namespace emscripten;
using namespace amamoto;

EMSCRIPTEN_BINDINGS(amamoto) {
    value_object<Vector2D>("Vector2D")
        .field("x", &Vector2D::x)
        .field("y", &Vector2D::y);
    
    class_<Simulation>("Simulation")
        .constructor<>()
        .function("createVehicle", &Simulation::createVehicle)
        .function("getVehicleCount", &Simulation::getVehicleCount)
        .function("getVehiclePosition", &Simulation::getVehiclePosition)
        .function("update", &Simulation::update)
        .function("clear", &Simulation::clear);
}
#endif