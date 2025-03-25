#include <emscripten/bind.h>
#include "TrafficSimulation.h"

using namespace emscripten;
using namespace amamoto;

// Class bindings for Emscripten
EMSCRIPTEN_BINDINGS(amamoto) {
    // Vector2D binding
    value_object<Vector2D>("Vector2D")
        .field("x", &Vector2D::x)
        .field("y", &Vector2D::y);
    
    // TrafficSimulation binding
    class_<TrafficSimulation>("TrafficSimulation")
        .constructor<>()
        .function("initialize", &TrafficSimulation::initialize)
        .function("createVehicle", &TrafficSimulation::createVehicle)
        .function("getVehicleCount", &TrafficSimulation::getVehicleCount)
        .function("getVehiclePosition", &TrafficSimulation::getVehiclePosition)
        .function("getVehicleVelocity", &TrafficSimulation::getVehicleVelocity)
        .function("update", &TrafficSimulation::update)
        .function("setKeepInBounds", &TrafficSimulation::setKeepInBounds)
        .function("getKeepInBounds", &TrafficSimulation::getKeepInBounds)
        .function("clear", &TrafficSimulation::clear)
        .function("reserveVehicles", &TrafficSimulation::reserveVehicles);
}

// Main entry point - doesn't need to do anything since we're using Emscripten bindings
int main() {
    return 0;
}