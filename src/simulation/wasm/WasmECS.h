#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <bitset>
#include <array>
#include <cassert>
#include <type_traits>
#include <functional>
#include <string>

namespace amamoto {

// Maximum number of component types
constexpr size_t MAX_COMPONENTS = 32;

// Entity ID type
using WasmEntityID = uint32_t;

// Component ID type
using WasmComponentID = uint8_t;

// Component mask to track which components an entity has
using WasmComponentMask = std::bitset<MAX_COMPONENTS>;

// Forward declarations
class WasmComponent;
class WasmEntity;
class WasmWorld;

// Base component class
class WasmComponent {
public:
    virtual ~WasmComponent() = default;
    
    // Get component type name (for debugging)
    virtual const char* getTypeName() const = 0;
};

// Entity class - essentially just an ID with a reference to its world
class WasmEntity {
public:
    WasmEntity() : m_id(0), m_world(nullptr), m_valid(false) {}
    WasmEntity(WasmEntityID id, WasmWorld* world) : m_id(id), m_world(world), m_valid(true) {}
    
    // Get entity ID
    WasmEntityID getID() const { return m_id; }
    
    // Check if entity is valid
    bool isValid() const { return m_valid; }
    
    // Check if entity has a component
    bool hasComponent(WasmComponentID componentId) const;
    
    // Add component to entity (implementation in WasmWorld)
    template<typename T, typename... Args>
    T& addComponent(Args&&... args);
    
    // Get component from entity (implementation in WasmWorld)
    template<typename T>
    T& getComponent();
    
    // Remove component from entity (implementation in WasmWorld)
    template<typename T>
    void removeComponent();
    
    // Equality operators
    bool operator==(const WasmEntity& other) const {
        return m_id == other.m_id && m_world == other.m_world && m_valid == other.m_valid;
    }
    
    bool operator!=(const WasmEntity& other) const {
        return !(*this == other);
    }
    
private:
    WasmEntityID m_id;
    WasmWorld* m_world;
    bool m_valid;
};

// Component registry for managing component types
class WasmComponentRegistry {
public:
    // Get component ID for a component type
    template<typename T>
    static WasmComponentID getComponentID() {
        static_assert(std::is_base_of<WasmComponent, T>::value, "T must derive from WasmComponent");
        static WasmComponentID typeID = nextComponentID++;
        assert(typeID < MAX_COMPONENTS && "Too many component types");
        return typeID;
    }
    
private:
    // Next component ID to assign
    static WasmComponentID nextComponentID;
};

// Component pool for storing components of a specific type
class WasmComponentPool {
public:
    virtual ~WasmComponentPool() = default;
    
    // Get component for entity
    virtual WasmComponent* getComponent(WasmEntityID entity) = 0;
    
    // Remove component for entity
    virtual void removeComponent(WasmEntityID entity) = 0;
    
    // Create a new component pool of this type
    virtual std::unique_ptr<WasmComponentPool> createPool() = 0;
    
    // Get the component type name
    virtual const char* getComponentTypeName() const = 0;
};

// Typed component pool for a specific component type
template<typename T>
class WasmTypedComponentPool : public WasmComponentPool {
public:
    WasmTypedComponentPool() = default;
    
    // Create a component for entity with given args
    template<typename... Args>
    T& createComponent(WasmEntityID entity, Args&&... args) {
        // Expand the array if needed
        if (entity >= m_components.size()) {
            m_components.resize(entity + 1);
            m_componentExists.resize(entity + 1, false);
        }
        
        // Create the component
        m_components[entity] = std::make_unique<T>(std::forward<Args>(args)...);
        m_componentExists[entity] = true;
        
        return *m_components[entity];
    }
    
    // Get component for entity
    WasmComponent* getComponent(WasmEntityID entity) override {
        if (entity < m_components.size() && m_componentExists[entity]) {
            return m_components[entity].get();
        }
        return nullptr;
    }
    
    // Remove component for entity
    void removeComponent(WasmEntityID entity) override {
        if (entity < m_components.size()) {
            m_components[entity].reset();
            m_componentExists[entity] = false;
        }
    }
    
    // Create a new component pool of this type
    std::unique_ptr<WasmComponentPool> createPool() override {
        return std::make_unique<WasmTypedComponentPool<T>>();
    }
    
    // Get the component type name
    const char* getComponentTypeName() const override {
        static T dummy;
        return dummy.getTypeName();
    }
    
private:
    std::vector<std::unique_ptr<T>> m_components;
    std::vector<bool> m_componentExists;
};

// Main ECS world class
class WasmWorld {
public:
    WasmWorld();
    ~WasmWorld();
    
    // Create a new entity
    WasmEntity createEntity();
    
    // Destroy an entity
    void destroyEntity(WasmEntity entity);
    
    // Check if an entity exists
    bool entityExists(WasmEntityID id) const;
    
    // Add a component to an entity
    template<typename T, typename... Args>
    T& addComponent(WasmEntityID entity, Args&&... args) {
        const WasmComponentID componentID = WasmComponentRegistry::getComponentID<T>();
        
        // Make sure entity exists
        assert(entityExists(entity) && "Entity does not exist");
        
        // Create the component pool if it doesn't exist
        if (componentID >= m_componentPools.size()) {
            m_componentPools.resize(componentID + 1);
        }
        
        if (!m_componentPools[componentID]) {
            m_componentPools[componentID] = std::make_unique<WasmTypedComponentPool<T>>();
        }
        
        // Add the component to the entity's mask
        m_entityMasks[entity].set(componentID);
        
        // Create the component
        auto pool = static_cast<WasmTypedComponentPool<T>*>(m_componentPools[componentID].get());
        return pool->createComponent(entity, std::forward<Args>(args)...);
    }
    
    // Get a component from an entity
    template<typename T>
    T& getComponent(WasmEntityID entity) {
        const WasmComponentID componentID = WasmComponentRegistry::getComponentID<T>();
        
        // Make sure entity exists and has the component
        assert(entityExists(entity) && "Entity does not exist");
        assert(m_entityMasks[entity].test(componentID) && "Entity does not have component");
        
        // Get the component
        auto pool = static_cast<WasmTypedComponentPool<T>*>(m_componentPools[componentID].get());
        auto component = static_cast<T*>(pool->getComponent(entity));
        
        assert(component && "Component not found");
        return *component;
    }
    
    // Get a component from an entity (const version)
    template<typename T>
    const T& getComponent(WasmEntityID entity) const {
        const WasmComponentID componentID = WasmComponentRegistry::getComponentID<T>();
        
        // Make sure entity exists and has the component
        assert(entityExists(entity) && "Entity does not exist");
        assert(m_entityMasks[entity].test(componentID) && "Entity does not have component");
        
        // Get the component
        auto pool = static_cast<const WasmTypedComponentPool<T>*>(m_componentPools[componentID].get());
        auto component = static_cast<const T*>(pool->getComponent(entity));
        
        assert(component && "Component not found");
        return *component;
    }
    
    // Remove a component from an entity
    template<typename T>
    void removeComponent(WasmEntityID entity) {
        const WasmComponentID componentID = WasmComponentRegistry::getComponentID<T>();
        
        // Make sure entity exists
        if (!entityExists(entity)) return;
        
        // Remove the component from the entity's mask
        m_entityMasks[entity].reset(componentID);
        
        // Remove the component
        if (componentID < m_componentPools.size() && m_componentPools[componentID]) {
            m_componentPools[componentID]->removeComponent(entity);
        }
    }
    
    // Check if an entity has a component
    template<typename T>
    bool hasComponent(WasmEntityID entity) const {
        const WasmComponentID componentID = WasmComponentRegistry::getComponentID<T>();
        
        // Make sure entity exists
        if (!entityExists(entity)) return false;
        
        // Check the component mask
        return m_entityMasks[entity].test(componentID);
    }
    
    // Get all entities with a specific set of components
    template<typename... Components>
    std::vector<WasmEntity> getEntitiesWith() {
        std::vector<WasmEntity> entities;
        
        // Create a mask with the required components
        WasmComponentMask requiredMask;
        (requiredMask.set(WasmComponentRegistry::getComponentID<Components>()), ...);
        
        // Check all entities
        for (size_t i = 0; i < m_entityMasks.size(); i++) {
            if ((m_entityMasks[i] & requiredMask) == requiredMask) {
                entities.emplace_back(static_cast<WasmEntityID>(i), this);
            }
        }
        
        return entities;
    }
    
    // Update all systems
    void update(float dt);
    
    // Register a system to run during update
    void registerSystem(std::function<void(WasmWorld&, float)> system);
    
    // Get number of active entities
    size_t getEntityCount() const { return m_entityCount; }
    
private:
    // Entity management
    std::vector<WasmEntityID> m_entities;
    std::vector<WasmComponentMask> m_entityMasks;
    std::vector<WasmEntityID> m_freeEntities;
    size_t m_entityCount;
    
    // Component pools
    std::vector<std::unique_ptr<WasmComponentPool>> m_componentPools;
    
    // Systems
    std::vector<std::function<void(WasmWorld&, float)>> m_systems;
};

// Entity method implementations that depend on WasmWorld
template<typename T, typename... Args>
T& WasmEntity::addComponent(Args&&... args) {
    assert(m_world && "Entity has no world");
    return m_world->addComponent<T>(m_id, std::forward<Args>(args)...);
}

template<typename T>
T& WasmEntity::getComponent() {
    assert(m_world && "Entity has no world");
    return m_world->getComponent<T>(m_id);
}

template<typename T>
void WasmEntity::removeComponent() {
    assert(m_world && "Entity has no world");
    m_world->removeComponent<T>(m_id);
}

inline bool WasmEntity::hasComponent(WasmComponentID componentId) const {
    assert(m_world && "Entity has no world");
    if (!m_valid) return false;
    return m_world->entityExists(m_id) && m_world->m_entityMasks[m_id].test(componentId);
}

// Initialize static member
inline WasmComponentID WasmComponentRegistry::nextComponentID = 0;

} // namespace amamoto