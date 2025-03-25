#include <emscripten/bind.h>
#include "SimpleTrafficSimulation.h"

using namespace emscripten;
using namespace amamoto;

// Class bindings for Emscripten
EMSCRIPTEN_BINDINGS(amamoto_simple) {
    // WasmVector2D binding
    value_object<WasmVector2D>("Vector2D")
        .field("x", &WasmVector2D::x)
        .field("y", &WasmVector2D::y);
    
    // WasmTrafficSimulation binding
    class_<WasmTrafficSimulation>("TrafficSimulation")
        .constructor<>()
        .function("initialize", &WasmTrafficSimulation::initialize)
        .function("createVehicle", &WasmTrafficSimulation::createVehicle)
        .function("getVehicleCount", &WasmTrafficSimulation::getVehicleCount)
        .function("getVehiclePosition", &WasmTrafficSimulation::getVehiclePosition)
        .function("getVehicleVelocity", &WasmTrafficSimulation::getVehicleVelocity)
        .function("update", &WasmTrafficSimulation::update)
        .function("setKeepInBounds", &WasmTrafficSimulation::setKeepInBounds)
        .function("getKeepInBounds", &WasmTrafficSimulation::getKeepInBounds)
        .function("clear", &WasmTrafficSimulation::clear)
        .function("reserveVehicles", &WasmTrafficSimulation::reserveVehicles);
}

// Main entry point - doesn't need to do anything since we're using Emscripten bindings
int main() {
    return 0;
}