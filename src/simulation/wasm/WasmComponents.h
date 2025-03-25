#pragma once

#include "WasmECS.h"
#include "SimpleTrafficSimulation.h" // For WasmVector2D

namespace amamoto {

// Transform component - position, velocity, etc.
class WasmTransformComponent : public WasmComponent {
public:
    WasmTransformComponent(const WasmVector2D& position = WasmVector2D(),
                          const WasmVector2D& velocity = WasmVector2D())
        : position(position), velocity(velocity), rotation(0.0f) {}
    
    const char* getTypeName() const override { return "WasmTransformComponent"; }
    
    WasmVector2D position;
    WasmVector2D velocity;
    float rotation;  // In radians
};

// Vehicle component - vehicle-specific properties
class WasmVehicleComponent : public WasmComponent {
public:
    WasmVehicleComponent(float maxSpeed = 100.0f)
        : maxSpeed(maxSpeed), currentSpeed(0.0f), targetSpeed(0.0f),
          length(4.0f), width(2.0f), acceleration(0.0f), braking(0.0f) {}
    
    const char* getTypeName() const override { return "WasmVehicleComponent"; }
    
    float maxSpeed;      // Maximum speed in units/second
    float currentSpeed;  // Current speed in units/second
    float targetSpeed;   // Target speed in units/second
    float length;        // Vehicle length in units
    float width;         // Vehicle width in units
    float acceleration;  // Current acceleration in units/second^2
    float braking;       // Current braking force in units/second^2
    
    // Vehicle type (car, truck, bus, etc.)
    enum class VehicleType {
        Car,
        Truck,
        Bus,
        Motorcycle
    };
    
    VehicleType type = VehicleType::Car;
};

// Path following component - for vehicles following roads
class WasmPathFollowingComponent : public WasmComponent {
public:
    WasmPathFollowingComponent() 
        : currentPathIndex(0), distanceAlongCurrentSegment(0.0f) {}
    
    const char* getTypeName() const override { return "WasmPathFollowingComponent"; }
    
    // Set a new path
    void setPath(const std::vector<std::pair<WasmRoadSegmentID, WasmLaneID>>& newPath) {
        path = newPath;
        currentPathIndex = 0;
        distanceAlongCurrentSegment = 0.0f;
    }
    
    // Check if we've reached the destination
    bool hasReachedDestination() const {
        return path.empty() || currentPathIndex >= path.size();
    }
    
    // Path as a sequence of (roadSegmentID, laneID) pairs
    std::vector<std::pair<WasmRoadSegmentID, WasmLaneID>> path;
    
    // Current position in the path
    size_t currentPathIndex;
    
    // Distance along the current segment
    float distanceAlongCurrentSegment;
    
    // Calculate steering force to follow the path
    WasmVector2D calculateSteeringForce(
        const WasmVector2D& currentPos,
        const WasmVector2D& currentVel,
        const WasmVector2D& targetPos,
        float maxSpeed) const {
        
        // Desired velocity is towards the target at max speed
        WasmVector2D desiredDirection(targetPos.x - currentPos.x, targetPos.y - currentPos.y);
        float distance = std::sqrt(desiredDirection.x * desiredDirection.x + desiredDirection.y * desiredDirection.y);
        
        if (distance < 0.0001f) return WasmVector2D();  // Already at target
        
        desiredDirection.x /= distance;
        desiredDirection.y /= distance;
        
        WasmVector2D desiredVelocity(desiredDirection.x * maxSpeed, desiredDirection.y * maxSpeed);
        
        // Steering force is the difference between desired and current velocity
        return WasmVector2D(
            desiredVelocity.x - currentVel.x,
            desiredVelocity.y - currentVel.y
        );
    }
};

// Bounds component - for keeping entities within bounds
class WasmBoundsComponent : public WasmComponent {
public:
    WasmBoundsComponent(float width = 800.0f, float height = 600.0f)
        : width(width), height(height), keepInBounds(true) {}
    
    const char* getTypeName() const override { return "WasmBoundsComponent"; }
    
    float width;
    float height;
    bool keepInBounds;
};

// Collision component - for collision detection
class WasmCollisionComponent : public WasmComponent {
public:
    WasmCollisionComponent(float radius = 1.0f) 
        : radius(radius), colliding(false) {}
    
    const char* getTypeName() const override { return "WasmCollisionComponent"; }
    
    float radius;
    bool colliding;
    std::vector<WasmEntityID> collidingWith;
};

// Renderable component - for rendering entities
class WasmRenderableComponent : public WasmComponent {
public:
    WasmRenderableComponent(float r = 0.2f, float g = 0.6f, float b = 0.8f, float a = 1.0f)
        : color{r, g, b, a}, scale(1.0f), visible(true), shape(Shape::Rectangle) {}
    
    const char* getTypeName() const override { return "WasmRenderableComponent"; }
    
    enum class Shape {
        Circle,
        Rectangle,
        Triangle,
        Custom
    };
    
    Shape shape;
    float color[4]; // RGBA
    float scale;
    bool visible;
    
    // For custom shapes, store vertices
    std::vector<WasmVector2D> vertices;
};

// Traffic signal component - for intersections with traffic signals
class WasmTrafficSignalComponent : public WasmComponent {
public:
    WasmTrafficSignalComponent()
        : state(State::Red), timeRemaining(0.0f), 
          greenDuration(30.0f), yellowDuration(5.0f), redDuration(30.0f) {}
    
    const char* getTypeName() const override { return "WasmTrafficSignalComponent"; }
    
    enum class State {
        Green,
        Yellow,
        Red
    };
    
    State state;
    float timeRemaining;      // Time until next state change
    float greenDuration;      // Duration of green light
    float yellowDuration;     // Duration of yellow light
    float redDuration;        // Duration of red light
    
    // Update the traffic signal
    void update(float dt) {
        timeRemaining -= dt;
        
        if (timeRemaining <= 0.0f) {
            // Change state
            switch (state) {
                case State::Green:
                    state = State::Yellow;
                    timeRemaining = yellowDuration;
                    break;
                    
                case State::Yellow:
                    state = State::Red;
                    timeRemaining = redDuration;
                    break;
                    
                case State::Red:
                    state = State::Green;
                    timeRemaining = greenDuration;
                    break;
            }
        }
    }
};

// Tag component for entities that can be selected
class WasmSelectableComponent : public WasmComponent {
public:
    WasmSelectableComponent(bool selected = false) : selected(selected) {}
    
    const char* getTypeName() const override { return "WasmSelectableComponent"; }
    
    bool selected;
};

} // namespace amamoto