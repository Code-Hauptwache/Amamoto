#pragma once

#include "Entity.h"
#include <cmath>

namespace amamoto {

// Vector2D struct for positions and velocities
struct Vector2D {
    float x = 0.0f;
    float y = 0.0f;
    
    Vector2D() = default;
    Vector2D(float x_, float y_) : x(x_), y(y_) {}
    
    // Vector operations
    Vector2D operator+(const Vector2D& other) const {
        return {x + other.x, y + other.y};
    }
    
    Vector2D operator-(const Vector2D& other) const {
        return {x - other.x, y - other.y};
    }
    
    Vector2D operator*(float scalar) const {
        return {x * scalar, y * scalar};
    }
    
    Vector2D& operator+=(const Vector2D& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    
    float length() const {
        return std::sqrt(x * x + y * y);
    }
    
    float lengthSquared() const {
        return x * x + y * y;
    }
    
    Vector2D normalized() const {
        float len = length();
        if (len < 0.0001f) return {0.0f, 0.0f};
        return {x / len, y / len};
    }
    
    float dot(const Vector2D& other) const {
        return x * other.x + y * other.y;
    }
};

// Transform component for position and movement
class TransformComponent : public Component {
public:
    TransformComponent(const Vector2D& position = {0, 0}, const Vector2D& velocity = {0, 0})
        : position(position), velocity(velocity) {}
    
    Vector2D position;
    Vector2D velocity;
    
    // Update position based on velocity
    void update(float dt) {
        position += velocity * dt;
    }
    
    // Keep within boundaries if specified
    void keepInBounds(float width, float height) {
        // Bounce off edges with reversed velocity
        if (position.x < 0) {
            position.x = 0;
            velocity.x = -velocity.x;
        }
        else if (position.x > width) {
            position.x = width;
            velocity.x = -velocity.x;
        }
        
        if (position.y < 0) {
            position.y = 0;
            velocity.y = -velocity.y;
        }
        else if (position.y > height) {
            position.y = height;
            velocity.y = -velocity.y;
        }
    }
};

// Vehicle component for vehicle-specific properties
class VehicleComponent : public Component {
public:
    VehicleComponent(
        float maxSpeed = 100.0f,
        float acceleration = 20.0f,
        float brakingForce = 40.0f,
        float mass = 1000.0f,
        float length = 4.5f,
        float width = 2.0f
    )
        : maxSpeed(maxSpeed)
        , acceleration(acceleration)
        , brakingForce(brakingForce)
        , mass(mass)
        , length(length)
        , width(width)
        , currentSpeed(0.0f)
        , targetSpeed(0.0f)
    {}
    
    float maxSpeed;        // Maximum speed in units/second
    float acceleration;    // Acceleration rate in units/second²
    float brakingForce;    // Braking force in units/second²
    float mass;            // Mass in kg
    float length;          // Vehicle length in units
    float width;           // Vehicle width in units
    float currentSpeed;    // Current speed in units/second
    float targetSpeed;     // Target speed in units/second
    
    // Bounding circle radius for simple collision detection
    float getBoundingRadius() const {
        return std::max(length, width) * 0.5f;
    }
    
    // Update vehicle physics
    void update(float dt, TransformComponent& transform) {
        // Simple implementation - just moves towards target speed
        if (currentSpeed < targetSpeed) {
            currentSpeed += acceleration * dt;
            if (currentSpeed > targetSpeed) {
                currentSpeed = targetSpeed;
            }
        }
        else if (currentSpeed > targetSpeed) {
            currentSpeed -= brakingForce * dt;
            if (currentSpeed < targetSpeed) {
                currentSpeed = targetSpeed;
            }
        }
        
        // Set velocity based on current speed and direction
        Vector2D direction = transform.velocity.normalized();
        transform.velocity = direction * currentSpeed;
    }
};

// Bounds component for simulation area
struct BoundsComponent : public Component {
    BoundsComponent(float width = 0, float height = 0)
        : width(width), height(height), keepEntitiesInBounds(true) {}
    
    float width;
    float height;
    bool keepEntitiesInBounds;
};

} // namespace amamoto