#include "TrafficSimulation.h"

namespace amamoto {

TrafficSimulation::TrafficSimulation()
    : m_world(std::make_unique<World>())
    , m_width(0)
    , m_height(0)
    , m_keepInBounds(true)
    , m_nextSimID(0)
    , m_initialized(false)
{
}

TrafficSimulation::~TrafficSimulation() {
    clear();
}

void TrafficSimulation::initialize(float width, float height) {
    m_width = width;
    m_height = height;
    m_initialized = true;
    
    // Create a bounds component for the world
    Entity boundsEntity = m_world->createEntity();
    boundsEntity.addComponent<BoundsComponent>(width, height);
}

int TrafficSimulation::createVehicle(float x, float y, float vx, float vy) {
    if (!m_initialized) {
        return -1;
    }
    
    // Create entity
    Entity entity = m_world->createEntity();
    
    // Add components
    entity.addComponent<TransformComponent>(Vector2D(x, y), Vector2D(vx, vy));
    entity.addComponent<VehicleComponent>();
    
    // Assign simulation ID
    int simID = m_nextSimID++;
    m_entityToSimID[entity.getID()] = simID;
    m_simIDToEntity[simID] = entity.getID();
    
    return simID;
}

int TrafficSimulation::getVehicleCount() const {
    return static_cast<int>(m_simIDToEntity.size());
}

Vector2D TrafficSimulation::getVehiclePosition(int id) const {
    if (!m_initialized || m_simIDToEntity.find(id) == m_simIDToEntity.end()) {
        return Vector2D(0, 0);
    }
    
    EntityID entityID = m_simIDToEntity.at(id);
    if (!m_world->entityExists(entityID)) {
        return Vector2D(0, 0);
    }
    
    Entity entity(entityID, m_world.get());
    if (!entity.hasComponent<TransformComponent>()) {
        return Vector2D(0, 0);
    }
    
    return entity.getComponent<TransformComponent>().position;
}

Vector2D TrafficSimulation::getVehicleVelocity(int id) const {
    if (!m_initialized || m_simIDToEntity.find(id) == m_simIDToEntity.end()) {
        return Vector2D(0, 0);
    }
    
    EntityID entityID = m_simIDToEntity.at(id);
    if (!m_world->entityExists(entityID)) {
        return Vector2D(0, 0);
    }
    
    Entity entity(entityID, m_world.get());
    if (!entity.hasComponent<TransformComponent>()) {
        return Vector2D(0, 0);
    }
    
    return entity.getComponent<TransformComponent>().velocity;
}

void TrafficSimulation::update(float dt) {
    if (!m_initialized) {
        return;
    }
    
    // Update vehicle transforms
    updateVehicleTransforms(dt);
    
    // Handle bounds if needed
    if (m_keepInBounds) {
        handleBounds();
    }
    
    // Update the world (will be expanded with more systems later)
    m_world->update(dt);
}

void TrafficSimulation::setKeepInBounds(bool keepInBounds) {
    m_keepInBounds = keepInBounds;
}

bool TrafficSimulation::getKeepInBounds() const {
    return m_keepInBounds;
}

void TrafficSimulation::clear() {
    if (!m_initialized) {
        return;
    }
    
    // Create a new world
    m_world = std::make_unique<World>();
    m_entityToSimID.clear();
    m_simIDToEntity.clear();
    m_nextSimID = 0;
    
    // Reinitialize with the same dimensions
    initialize(m_width, m_height);
}

void TrafficSimulation::reserveVehicles(int count) {
    // Pre-allocate space for the given number of vehicles
    m_entityToSimID.reserve(count);
    m_simIDToEntity.reserve(count);
}

void TrafficSimulation::updateVehicleTransforms(float dt) {
    // Get all entities with transform and vehicle components
    auto entities = m_world->getEntitiesWith<TransformComponent, VehicleComponent>();
    
    for (auto& entity : entities) {
        auto& transform = entity.getComponent<TransformComponent>();
        auto& vehicle = entity.getComponent<VehicleComponent>();
        
        // Update vehicle physics
        vehicle.update(dt, transform);
        
        // Update position based on velocity
        transform.update(dt);
    }
}

void TrafficSimulation::handleBounds() {
    // Get all entities with transform components
    auto entities = m_world->getEntitiesWith<TransformComponent>();
    
    for (auto& entity : entities) {
        auto& transform = entity.getComponent<TransformComponent>();
        
        // Keep within boundaries
        transform.keepInBounds(m_width, m_height);
    }
}

#ifdef __EMSCRIPTEN__
// Implement the C functions for Emscripten binding

TrafficSimulation* TrafficSimulation_Create() {
    return new TrafficSimulation();
}

void TrafficSimulation_Destroy(TrafficSimulation* simulation) {
    delete simulation;
}

void TrafficSimulation_Initialize(TrafficSimulation* simulation, float width, float height) {
    simulation->initialize(width, height);
}

int TrafficSimulation_CreateVehicle(TrafficSimulation* simulation, float x, float y, float vx, float vy) {
    return simulation->createVehicle(x, y, vx, vy);
}

int TrafficSimulation_GetVehicleCount(TrafficSimulation* simulation) {
    return simulation->getVehicleCount();
}

void TrafficSimulation_GetVehiclePosition(TrafficSimulation* simulation, int id, float* x, float* y) {
    Vector2D pos = simulation->getVehiclePosition(id);
    *x = pos.x;
    *y = pos.y;
}

void TrafficSimulation_GetVehicleVelocity(TrafficSimulation* simulation, int id, float* vx, float* vy) {
    Vector2D vel = simulation->getVehicleVelocity(id);
    *vx = vel.x;
    *vy = vel.y;
}

void TrafficSimulation_Update(TrafficSimulation* simulation, float dt) {
    simulation->update(dt);
}

void TrafficSimulation_SetKeepInBounds(TrafficSimulation* simulation, bool keepInBounds) {
    simulation->setKeepInBounds(keepInBounds);
}

bool TrafficSimulation_GetKeepInBounds(TrafficSimulation* simulation) {
    return simulation->getKeepInBounds();
}

void TrafficSimulation_Clear(TrafficSimulation* simulation) {
    simulation->clear();
}

void TrafficSimulation_ReserveVehicles(TrafficSimulation* simulation, int count) {
    simulation->reserveVehicles(count);
}
#endif

} // namespace amamoto