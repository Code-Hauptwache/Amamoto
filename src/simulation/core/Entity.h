#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <memory>
#include <unordered_map>
#include <bitset>

namespace amamoto {

// Forward declarations
class World;

// Entity ID type
using EntityID = uint32_t;

// Component ID type for efficient component type checking
using ComponentTypeID = uint8_t;
constexpr ComponentTypeID MAX_COMPONENT_TYPES = 32;

// Component mask to track which components an entity has
using ComponentMask = std::bitset<MAX_COMPONENT_TYPES>;

// Base Component class
class Component {
public:
    virtual ~Component() = default;
};

// Entity class - lightweight handle to entities
class Entity {
public:
    Entity() : m_id(0), m_world(nullptr) {}
    Entity(EntityID id, World* world) : m_id(id), m_world(world) {}
    
    // Get entity ID
    EntityID getID() const { return m_id; }
    
    // Check if entity is valid
    bool isValid() const { return m_world != nullptr; }
    
    // Add component to entity
    template<typename T, typename... Args>
    T& addComponent(Args&&... args);
    
    // Remove component from entity
    template<typename T>
    void removeComponent();
    
    // Check if entity has component
    template<typename T>
    bool hasComponent() const;
    
    // Get component from entity
    template<typename T>
    T& getComponent();
    
    // Equality operators
    bool operator==(const Entity& other) const {
        return m_id == other.m_id && m_world == other.m_world;
    }
    
    bool operator!=(const Entity& other) const {
        return !(*this == other);
    }
    
private:
    EntityID m_id;
    World* m_world;
};

// World class - manages all entities and components
class World {
    friend class Entity;
    
public:
    World();
    ~World();
    
    // Create a new entity
    Entity createEntity();
    
    // Destroy an entity
    void destroyEntity(Entity entity);
    
    // Check if entity exists
    bool entityExists(EntityID id) const;
    
    // Get all entities with specific components
    template<typename... Components>
    std::vector<Entity> getEntitiesWith();
    
    // Update all entities (called each frame)
    void update(float dt);
    
    // Get entity count
    size_t getEntityCount() const { return m_entityCount; }
    
private:
    // Component management methods (implementation details)
    template<typename T>
    ComponentTypeID getComponentTypeID() const;
    
    template<typename T, typename... Args>
    T& addComponent(EntityID entity, Args&&... args);
    
    template<typename T>
    void removeComponent(EntityID entity);
    
    template<typename T>
    bool hasComponent(EntityID entity) const;
    
    template<typename T>
    T& getComponent(EntityID entity);
    
    // Entity management
    std::vector<EntityID> m_entities;
    std::vector<EntityID> m_freeIDs;
    size_t m_entityCount;
    
    // Component storage
    std::vector<std::unique_ptr<std::vector<std::unique_ptr<Component>>>> m_components;
    std::vector<ComponentMask> m_entityMasks;
    
    // Component type ID counter
    static ComponentTypeID s_nextComponentTypeID;
    
    // Friend function to allow access to component type ID counter
    template<typename T>
    friend ComponentTypeID getComponentTypeID();
};

// Helper functions to get component type IDs
template<typename T>
ComponentTypeID getComponentTypeID() {
    static ComponentTypeID typeID = World::s_nextComponentTypeID++;
    return typeID;
}

// Implementation of Entity template methods
template<typename T, typename... Args>
T& Entity::addComponent(Args&&... args) {
    return m_world->addComponent<T>(m_id, std::forward<Args>(args)...);
}

template<typename T>
void Entity::removeComponent() {
    m_world->removeComponent<T>(m_id);
}

template<typename T>
bool Entity::hasComponent() const {
    return m_world->hasComponent<T>(m_id);
}

template<typename T>
T& Entity::getComponent() {
    return m_world->getComponent<T>(m_id);
}

} // namespace amamoto