#include <emscripten/bind.h>
#include "WasmTrafficSimulation.h"

using namespace emscripten;

// Class bindings for Emscripten
EMSCRIPTEN_BINDINGS(amamoto_ecs) {
    // WasmVector2D binding
    value_object<amamoto::WasmVector2D>("Vector2D")
        .field("x", &amamoto::WasmVector2D::x)
        .field("y", &amamoto::WasmVector2D::y);
    
    // WasmEntityID binding as a value
    class_<amamoto::WasmEntity>("Entity")
        .property("id", &amamoto::WasmEntity::getID)
        .property("valid", &amamoto::WasmEntity::isValid);
    
    // Component bindings
    class_<amamoto::WasmComponent>("Component")
        .function("getTypeName", &amamoto::WasmComponent::getTypeName);
    
    class_<amamoto::WasmTransformComponent, base<amamoto::WasmComponent>>("TransformComponent")
        .constructor<const amamoto::WasmVector2D&, const amamoto::WasmVector2D&>()
        .property("position", &amamoto::WasmTransformComponent::position)
        .property("velocity", &amamoto::WasmTransformComponent::velocity)
        .property("rotation", &amamoto::WasmTransformComponent::rotation);
    
    class_<amamoto::WasmVehicleComponent, base<amamoto::WasmComponent>>("VehicleComponent")
        .constructor<float>()
        .property("maxSpeed", &amamoto::WasmVehicleComponent::maxSpeed)
        .property("currentSpeed", &amamoto::WasmVehicleComponent::currentSpeed)
        .property("targetSpeed", &amamoto::WasmVehicleComponent::targetSpeed);
    
    // WasmTrafficSimulation binding
    class_<amamoto::WasmTrafficSimulation>("TrafficSimulation")
        .constructor<>()
        .function("initialize", &amamoto::WasmTrafficSimulation::initialize)
        .function("createVehicle", &amamoto::WasmTrafficSimulation::createVehicle)
        .function("getVehicleCount", &amamoto::WasmTrafficSimulation::getVehicleCount)
        .function("getVehiclePosition", &amamoto::WasmTrafficSimulation::getVehiclePosition)
        .function("getVehicleVelocity", &amamoto::WasmTrafficSimulation::getVehicleVelocity)
        .function("update", &amamoto::WasmTrafficSimulation::update)
        .function("setKeepInBounds", &amamoto::WasmTrafficSimulation::setKeepInBounds)
        .function("getKeepInBounds", &amamoto::WasmTrafficSimulation::getKeepInBounds)
        .function("clear", &amamoto::WasmTrafficSimulation::clear)
        .function("reserveVehicles", &amamoto::WasmTrafficSimulation::reserveVehicles)
        .function("createPath", &amamoto::WasmTrafficSimulation::createPath);
    
    // WasmRoadNetwork binding
    class_<amamoto::WasmRoadNetwork>("RoadNetwork")
        .constructor<>()
        .function("createRoadSegment", &amamoto::WasmRoadNetwork::createRoadSegment)
        .function("createIntersection", &amamoto::WasmRoadNetwork::createIntersection)
        .function("connectWithIntersection", &amamoto::WasmRoadNetwork::connectWithIntersection)
        .function("findNearestRoadSegment", &amamoto::WasmRoadNetwork::findNearestRoadSegment)
        .function("findNearestIntersection", &amamoto::WasmRoadNetwork::findNearestIntersection)
        .function("loadFromJSON", &amamoto::WasmRoadNetwork::loadFromJSON)
        .function("exportToJSON", &amamoto::WasmRoadNetwork::exportToJSON)
        .function("clear", &amamoto::WasmRoadNetwork::clear);
}

// Main entry point - doesn't need to do anything since we're using Emscripten bindings
int main() {
    return 0;
}