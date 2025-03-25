#include "WasmTrafficSimulation.h"
#include <cmath>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace amamoto {

WasmTrafficSimulation::WasmTrafficSimulation()
    : m_roadNetwork(nullptr)
    , m_width(800.0f)
    , m_height(600.0f)
    , m_keepInBounds(true)
{
    // Set up ECS systems
    setupSystems();
}

WasmTrafficSimulation::~WasmTrafficSimulation() {
    // Clear all entities
    clear();
}

void WasmTrafficSimulation::initialize(float width, float height) {
    m_width = width;
    m_height = height;
    
    // Create a bounds component for the simulation
    auto entity = m_world.createEntity();
    m_world.addComponent<WasmBoundsComponent>(entity.getID(), width, height);
}

WasmEntityID WasmTrafficSimulation::createVehicle(float x, float y, float vx, float vy) {
    // Create a new entity
    auto entity = m_world.createEntity();
    WasmEntityID id = entity.getID();
    
    // Add components
    m_world.addComponent<WasmTransformComponent>(id, WasmVector2D(x, y), WasmVector2D(vx, vy));
    m_world.addComponent<WasmVehicleComponent>(id);
    m_world.addComponent<WasmCollisionComponent>(id, 2.0f); // Collision radius
    m_world.addComponent<WasmRenderableComponent>(id);
    
    return id;
}

int WasmTrafficSimulation::getVehicleCount() const {
    // Count entities with TransformComponent and VehicleComponent
    int count = 0;
    for (WasmEntityID id = 0; id < m_world.getEntityCount(); id++) {
        if (m_world.entityExists(id) && 
            m_world.hasComponent<WasmTransformComponent>(id) && 
            m_world.hasComponent<WasmVehicleComponent>(id)) {
            count++;
        }
    }
    return count;
}

WasmVector2D WasmTrafficSimulation::getVehiclePosition(WasmEntityID id) const {
    if (m_world.entityExists(id) && m_world.hasComponent<WasmTransformComponent>(id)) {
        const auto& transform = m_world.getComponent<WasmTransformComponent>(id);
        return transform.position;
    }
    return WasmVector2D(0, 0);
}

WasmVector2D WasmTrafficSimulation::getVehicleVelocity(WasmEntityID id) const {
    if (m_world.entityExists(id) && m_world.hasComponent<WasmTransformComponent>(id)) {
        const auto& transform = m_world.getComponent<WasmTransformComponent>(id);
        return transform.velocity;
    }
    return WasmVector2D(0, 0);
}

void WasmTrafficSimulation::update(float dt) {
    // Update the ECS world
    m_world.update(dt);
}

void WasmTrafficSimulation::setKeepInBounds(bool keepInBounds) {
    m_keepInBounds = keepInBounds;
    
    // Update all bounds components
    for (WasmEntityID id = 0; id < m_world.getEntityCount(); id++) {
        if (m_world.entityExists(id) && m_world.hasComponent<WasmBoundsComponent>(id)) {
            auto& bounds = m_world.getComponent<WasmBoundsComponent>(id);
            bounds.keepInBounds = keepInBounds;
        }
    }
}

bool WasmTrafficSimulation::getKeepInBounds() const {
    return m_keepInBounds;
}

void WasmTrafficSimulation::setRoadNetwork(WasmRoadNetwork* roadNetwork) {
    m_roadNetwork = roadNetwork;
    
    // Re-register the path following system with the new road network
    m_world.registerSystem(WasmSystemWrapper::createPathFollowingSystem(m_roadNetwork));
}

WasmRoadNetwork* WasmTrafficSimulation::getRoadNetwork() const {
    return m_roadNetwork;
}

bool WasmTrafficSimulation::createPath(WasmEntityID vehicleId, float startX, float startY, float endX, float endY) {
    if (!m_roadNetwork || !m_world.entityExists(vehicleId)) {
        return false;
    }
    
    // Find a path between the points
    auto path = findPath(startX, startY, endX, endY);
    
    if (path.empty()) {
        return false;
    }
    
    // Add a path following component to the vehicle if it doesn't have one
    if (!m_world.hasComponent<WasmPathFollowingComponent>(vehicleId)) {
        m_world.addComponent<WasmPathFollowingComponent>(vehicleId);
    }
    
    // Set the path
    auto& pathComponent = m_world.getComponent<WasmPathFollowingComponent>(vehicleId);
    pathComponent.setPath(path);
    
    return true;
}

void WasmTrafficSimulation::clear() {
    // Destroy all entities
    for (WasmEntityID id = 0; id < m_world.getEntityCount(); id++) {
        if (m_world.entityExists(id)) {
            m_world.destroyEntity(WasmEntity(id, &m_world));
        }
    }
}

void WasmTrafficSimulation::reserveVehicles(int count) {
    // No direct way to reserve entities in the ECS, but we can create them and immediately destroy them
    // to populate the free list
    std::vector<WasmEntity> entities;
    for (int i = 0; i < count; i++) {
        entities.push_back(m_world.createEntity());
    }
    
    for (auto& entity : entities) {
        m_world.destroyEntity(entity);
    }
}

void WasmTrafficSimulation::setupSystems() {
    // Register systems in update order
    m_world.registerSystem([this](WasmWorld& world, float dt) {
        movementSystem(world, dt);
    });
    
    m_world.registerSystem([this](WasmWorld& world, float dt) {
        boundsSystem(world, dt);
    });
    
    // Path following system will be registered when road network is set
    
    m_world.registerSystem([this](WasmWorld& world, float dt) {
        collisionSystem(world, dt);
    });
    
    m_world.registerSystem([this](WasmWorld& world, float dt) {
        trafficSignalSystem(world, dt);
    });
}

void WasmTrafficSimulation::movementSystem(WasmWorld& world, float dt) {
    // Update positions based on velocities
    auto entities = world.getEntitiesWith<WasmTransformComponent>();
    
    for (auto& entity : entities) {
        auto& transform = world.getComponent<WasmTransformComponent>(entity.getID());
        
        // Update position
        transform.position.x += transform.velocity.x * dt;
        transform.position.y += transform.velocity.y * dt;
        
        // Update rotation based on velocity direction
        if (std::abs(transform.velocity.x) > 0.1f || std::abs(transform.velocity.y) > 0.1f) {
            transform.rotation = std::atan2(transform.velocity.y, transform.velocity.x);
        }
    }
}

void WasmTrafficSimulation::boundsSystem(WasmWorld& world, float dt) {
    // Find the bounds component
    WasmBoundsComponent* globalBounds = nullptr;
    
    for (WasmEntityID id = 0; id < world.getEntityCount(); id++) {
        if (world.entityExists(id) && world.hasComponent<WasmBoundsComponent>(id)) {
            globalBounds = &world.getComponent<WasmBoundsComponent>(id);
            break;
        }
    }
    
    if (!globalBounds || !globalBounds->keepInBounds) {
        return;
    }
    
    // Keep vehicles within bounds
    auto entities = world.getEntitiesWith<WasmTransformComponent, WasmVehicleComponent>();
    
    for (auto& entity : entities) {
        auto& transform = world.getComponent<WasmTransformComponent>(entity.getID());
        auto& vehicle = world.getComponent<WasmVehicleComponent>(entity.getID());
        
        // Check bounds for x
        if (transform.position.x < 0) {
            transform.position.x = 0;
            transform.velocity.x = -transform.velocity.x * 0.5f; // Bounce with energy loss
        }
        else if (transform.position.x > globalBounds->width) {
            transform.position.x = globalBounds->width;
            transform.velocity.x = -transform.velocity.x * 0.5f; // Bounce with energy loss
        }
        
        // Check bounds for y
        if (transform.position.y < 0) {
            transform.position.y = 0;
            transform.velocity.y = -transform.velocity.y * 0.5f; // Bounce with energy loss
        }
        else if (transform.position.y > globalBounds->height) {
            transform.position.y = globalBounds->height;
            transform.velocity.y = -transform.velocity.y * 0.5f; // Bounce with energy loss
        }
    }
}

void WasmTrafficSimulation::pathFollowingSystem(WasmWorld& world, float dt, WasmRoadNetwork* roadNetwork) {
    if (!roadNetwork) {
        return;
    }
    
    auto entities = world.getEntitiesWith<WasmTransformComponent, WasmVehicleComponent, WasmPathFollowingComponent>();
    
    for (auto& entity : entities) {
        auto& transform = world.getComponent<WasmTransformComponent>(entity.getID());
        auto& vehicle = world.getComponent<WasmVehicleComponent>(entity.getID());
        auto& pathFollowing = world.getComponent<WasmPathFollowingComponent>(entity.getID());
        
        if (pathFollowing.hasReachedDestination()) {
            // Stop the vehicle
            vehicle.targetSpeed = 0.0f;
            continue;
        }
        
        // Get current road segment and lane
        auto [roadID, laneID] = pathFollowing.path[pathFollowing.currentPathIndex];
        WasmRoadSegment* road = roadNetwork->getRoadSegment(roadID);
        
        if (!road) {
            // Invalid road, skip to next
            pathFollowing.currentPathIndex++;
            continue;
        }
        
        // Calculate target position
        float lookAheadDistance = vehicle.currentSpeed * 2.0f + 5.0f; // Look ahead more at higher speeds
        float targetDistance = pathFollowing.distanceAlongCurrentSegment + lookAheadDistance;
        
        // Check if we need to move to the next road segment
        if (targetDistance > road->getLength()) {
            // We've reached the end of this segment
            pathFollowing.currentPathIndex++;
            
            if (pathFollowing.hasReachedDestination()) {
                // Stop the vehicle
                vehicle.targetSpeed = 0.0f;
                continue;
            }
            
            // Reset distance for the next segment
            pathFollowing.distanceAlongCurrentSegment = 0.0f;
            targetDistance = lookAheadDistance;
            
            // Get the new road segment
            auto [nextRoadID, nextLaneID] = pathFollowing.path[pathFollowing.currentPathIndex];
            road = roadNetwork->getRoadSegment(nextRoadID);
            
            if (!road) {
                // Invalid road
                continue;
            }
        }
        
        // Get target position and direction
        WasmVector2D targetPosition = road->getPointAtDistance(targetDistance);
        WasmVector2D targetDirection = road->getDirectionAtDistance(targetDistance);
        
        // Calculate steering force
        WasmVector2D steeringForce = pathFollowing.calculateSteeringForce(
            transform.position,
            transform.velocity,
            targetPosition,
            vehicle.maxSpeed
        );
        
        // Apply steering
        transform.velocity.x += steeringForce.x * dt;
        transform.velocity.y += steeringForce.y * dt;
        
        // Limit velocity to max speed
        float speed = std::sqrt(transform.velocity.x * transform.velocity.x + transform.velocity.y * transform.velocity.y);
        if (speed > vehicle.maxSpeed) {
            float scale = vehicle.maxSpeed / speed;
            transform.velocity.x *= scale;
            transform.velocity.y *= scale;
        }
        
        // Update distance along segment
        vehicle.currentSpeed = speed;
        pathFollowing.distanceAlongCurrentSegment += vehicle.currentSpeed * dt;
        
        // Set target speed based on conditions (traffic signals, curves, etc.)
        vehicle.targetSpeed = vehicle.maxSpeed;
    }
}

void WasmTrafficSimulation::collisionSystem(WasmWorld& world, float dt) {
    // Simple collision detection between vehicles
    auto entities = world.getEntitiesWith<WasmTransformComponent, WasmCollisionComponent>();
    
    // Reset collision flags
    for (auto& entity : entities) {
        auto& collision = world.getComponent<WasmCollisionComponent>(entity.getID());
        collision.colliding = false;
        collision.collidingWith.clear();
    }
    
    // Check for collisions between all pairs
    for (size_t i = 0; i < entities.size(); i++) {
        for (size_t j = i + 1; j < entities.size(); j++) {
            auto& transformA = world.getComponent<WasmTransformComponent>(entities[i].getID());
            auto& transformB = world.getComponent<WasmTransformComponent>(entities[j].getID());
            
            auto& collisionA = world.getComponent<WasmCollisionComponent>(entities[i].getID());
            auto& collisionB = world.getComponent<WasmCollisionComponent>(entities[j].getID());
            
            // Calculate distance
            float dx = transformB.position.x - transformA.position.x;
            float dy = transformB.position.y - transformA.position.y;
            float distanceSquared = dx * dx + dy * dy;
            
            // Calculate minimum separation distance
            float minDistance = collisionA.radius + collisionB.radius;
            float minDistanceSquared = minDistance * minDistance;
            
            if (distanceSquared < minDistanceSquared) {
                // Collision detected
                collisionA.colliding = true;
                collisionB.colliding = true;
                
                collisionA.collidingWith.push_back(entities[j].getID());
                collisionB.collidingWith.push_back(entities[i].getID());
                
                // Calculate collision response
                float distance = std::sqrt(distanceSquared);
                float overlap = minDistance - distance;
                
                // Calculate normalized direction from A to B
                float nx = dx / distance;
                float ny = dy / distance;
                
                // Separate the entities
                float separationX = nx * overlap * 0.5f;
                float separationY = ny * overlap * 0.5f;
                
                transformA.position.x -= separationX;
                transformA.position.y -= separationY;
                
                transformB.position.x += separationX;
                transformB.position.y += separationY;
                
                // Only handle velocity response if both entities have vehicles
                if (world.hasComponent<WasmVehicleComponent>(entities[i].getID()) && 
                    world.hasComponent<WasmVehicleComponent>(entities[j].getID())) {
                    
                    // Exchange velocities (simplified elastic collision)
                    WasmVector2D temp = transformA.velocity;
                    transformA.velocity = transformB.velocity;
                    transformB.velocity = temp;
                    
                    // Apply some damping
                    transformA.velocity.x *= 0.9f;
                    transformA.velocity.y *= 0.9f;
                    transformB.velocity.x *= 0.9f;
                    transformB.velocity.y *= 0.9f;
                }
            }
        }
    }
}

void WasmTrafficSimulation::trafficSignalSystem(WasmWorld& world, float dt) {
    // Update all traffic signals
    auto entities = world.getEntitiesWith<WasmTrafficSignalComponent>();
    
    for (auto& entity : entities) {
        auto& signal = world.getComponent<WasmTrafficSignalComponent>(entity.getID());
        signal.update(dt);
    }
}

std::vector<std::pair<WasmRoadSegmentID, WasmLaneID>> WasmTrafficSimulation::findPath(
    float startX, float startY, float endX, float endY) {
    if (!m_roadNetwork) {
        return {};
    }
    
    // Find nearest road segments to start and end points
    WasmRoadSegmentID startRoadId = m_roadNetwork->findNearestRoadSegment(startX, startY);
    WasmRoadSegmentID endRoadId = m_roadNetwork->findNearestRoadSegment(endX, endY);
    
    if (startRoadId == static_cast<WasmRoadSegmentID>(-1) || 
        endRoadId == static_cast<WasmRoadSegmentID>(-1)) {
        return {};
    }
    
    // If start and end are on the same road, use it
    if (startRoadId == endRoadId) {
        return { {startRoadId, 0} };
    }
    
    // Simplified A* implementation
    struct Node {
        WasmRoadSegmentID roadId;
        float gScore;
        float fScore;
        Node* parent;
        
        Node(WasmRoadSegmentID id, float g, float f, Node* p) 
            : roadId(id), gScore(g), fScore(f), parent(p) {}
        
        bool operator<(const Node& other) const {
            return fScore > other.fScore; // For priority queue (min-heap)
        }
    };
    
    // Priority queue for open set
    std::priority_queue<Node> openSet;
    
    // Track visited nodes and their scores
    std::unordered_map<WasmRoadSegmentID, float> gScores;
    std::unordered_set<WasmRoadSegmentID> closedSet;
    std::unordered_map<WasmRoadSegmentID, Node*> nodeMap;
    
    // Start with the start road
    auto startRoad = m_roadNetwork->getRoadSegment(startRoadId);
    auto endRoad = m_roadNetwork->getRoadSegment(endRoadId);
    
    if (!startRoad || !endRoad) {
        return {};
    }
    
    // Calculate heuristic (straight-line distance between road midpoints)
    auto startMidpoint = WasmVector2D(
        (startRoad->getStartPoint().x + startRoad->getEndPoint().x) / 2,
        (startRoad->getStartPoint().y + startRoad->getEndPoint().y) / 2
    );
    
    auto endMidpoint = WasmVector2D(
        (endRoad->getStartPoint().x + endRoad->getEndPoint().x) / 2,
        (endRoad->getStartPoint().y + endRoad->getEndPoint().y) / 2
    );
    
    auto heuristic = [&endMidpoint](const WasmRoadSegment* road) -> float {
        WasmVector2D midpoint(
            (road->getStartPoint().x + road->getEndPoint().x) / 2,
            (road->getStartPoint().y + road->getEndPoint().y) / 2
        );
        
        float dx = midpoint.x - endMidpoint.x;
        float dy = midpoint.y - endMidpoint.y;
        return std::sqrt(dx * dx + dy * dy);
    };
    
    // Create start node
    Node* startNode = new Node(startRoadId, 0, heuristic(startRoad), nullptr);
    nodeMap[startRoadId] = startNode;
    openSet.push(*startNode);
    gScores[startRoadId] = 0;
    
    Node* endNode = nullptr;
    
    // A* search
    while (!openSet.empty()) {
        // Get the node with the lowest f-score
        Node current = openSet.top();
        openSet.pop();
        
        // Skip if we've already processed this node
        if (closedSet.count(current.roadId) > 0) {
            continue;
        }
        
        // Check if we've reached the end
        if (current.roadId == endRoadId) {
            endNode = nodeMap[current.roadId];
            break;
        }
        
        // Add to closed set
        closedSet.insert(current.roadId);
        
        // Get the current road segment
        auto road = m_roadNetwork->getRoadSegment(current.roadId);
        if (!road) continue;
        
        // Find connected roads through intersections
        // To simplify, we'll just check if the roads share an endpoint
        for (WasmRoadSegmentID neighborId = 0; neighborId < 100; neighborId++) {
            if (neighborId == current.roadId) continue;
            
            auto neighbor = m_roadNetwork->getRoadSegment(neighborId);
            if (!neighbor) continue;
            
            bool connected = false;
            
            // Check if roads share the same intersection
            if ((road->getStartIntersection() && road->getStartIntersection() == neighbor->getStartIntersection()) ||
                (road->getStartIntersection() && road->getStartIntersection() == neighbor->getEndIntersection()) ||
                (road->getEndIntersection() && road->getEndIntersection() == neighbor->getStartIntersection()) ||
                (road->getEndIntersection() && road->getEndIntersection() == neighbor->getEndIntersection())) {
                connected = true;
            }
            
            if (!connected) continue;
            
            // Skip if already in closed set
            if (closedSet.count(neighborId) > 0) continue;
            
            // Calculate g-score (distance from start)
            float tentativeGScore = gScores[current.roadId] + road->getLength();
            
            // Skip if we already have a better path
            if (gScores.count(neighborId) > 0 && tentativeGScore >= gScores[neighborId]) {
                continue;
            }
            
            // This is the best path so far
            if (nodeMap.count(neighborId) == 0) {
                nodeMap[neighborId] = new Node(neighborId, tentativeGScore, tentativeGScore + heuristic(neighbor), nodeMap[current.roadId]);
            } else {
                nodeMap[neighborId]->gScore = tentativeGScore;
                nodeMap[neighborId]->fScore = tentativeGScore + heuristic(neighbor);
                nodeMap[neighborId]->parent = nodeMap[current.roadId];
            }
            
            gScores[neighborId] = tentativeGScore;
            openSet.push(*nodeMap[neighborId]);
        }
    }
    
    // Reconstruct the path
    std::vector<std::pair<WasmRoadSegmentID, WasmLaneID>> path;
    
    if (endNode) {
        Node* current = endNode;
        while (current) {
            path.push_back({current->roadId, 0}); // Always use lane 0 for simplicity
            current = current->parent;
        }
        std::reverse(path.begin(), path.end());
    }
    
    // Clean up nodes
    for (auto& [id, node] : nodeMap) {
        delete node;
    }
    
    return path;
}

} // namespace amamoto