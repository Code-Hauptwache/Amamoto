#include "Entity.h"
#include "Components.h"

namespace amamoto {

// Initialize static component type ID counter
ComponentTypeID World::s_nextComponentTypeID = 0;

// World implementation
World::World() : m_entityCount(0) {
    // Reserve space for component storage to avoid reallocations
    m_components.reserve(MAX_COMPONENT_TYPES);
    for (size_t i = 0; i < MAX_COMPONENT_TYPES; i++) {
        m_components.push_back(std::make_unique<std::vector<std::unique_ptr<Component>>>());
    }
}

World::~World() {
    // Clear all entities and components
    m_entities.clear();
    m_freeIDs.clear();
    
    for (auto& componentArray : m_components) {
        componentArray->clear();
    }
    m_components.clear();
    
    m_entityMasks.clear();
}

Entity World::createEntity() {
    EntityID id;
    
    // Reuse an ID from the free list if available
    if (!m_freeIDs.empty()) {
        id = m_freeIDs.back();
        m_freeIDs.pop_back();
    } else {
        // Otherwise, create a new ID
        id = static_cast<EntityID>(m_entities.size());
        m_entities.push_back(id);
        m_entityMasks.push_back(ComponentMask());
        
        // Ensure component arrays are large enough
        for (auto& componentArray : m_components) {
            componentArray->resize(m_entities.size());
        }
    }
    
    m_entityCount++;
    return Entity(id, this);
}

void World::destroyEntity(Entity entity) {
    const EntityID id = entity.getID();
    
    if (!entityExists(id)) {
        return;
    }
    
    // Clear all components
    m_entityMasks[id].reset();
    
    // Add the ID to the free list for reuse
    m_freeIDs.push_back(id);
    m_entityCount--;
}

bool World::entityExists(EntityID id) const {
    return id < m_entities.size() && m_entityMasks[id].any();
}

void World::update(float dt) {
    // This method would update all systems and components
    // For now, it's a placeholder - specific behavior will be implemented
    // in derived simulation classes
}

template<typename T>
ComponentTypeID World::getComponentTypeID() const {
    return ::amamoto::getComponentTypeID<T>();
}

template<typename T, typename... Args>
T& World::addComponent(EntityID entity, Args&&... args) {
    // Get the component type ID
    const ComponentTypeID typeID = getComponentTypeID<T>();
    
    // Ensure the entity exists
    if (entity >= m_entities.size()) {
        throw std::out_of_range("Entity ID out of range");
    }
    
    // Create the component
    auto& componentArray = *m_components[typeID];
    
    // Ensure the component array is large enough
    if (componentArray.size() <= entity) {
        componentArray.resize(m_entities.size());
    }
    
    // Create the component
    componentArray[entity] = std::make_unique<T>(std::forward<Args>(args)...);
    
    // Set the component bit
    m_entityMasks[entity].set(typeID);
    
    // Return the component
    return *static_cast<T*>(componentArray[entity].get());
}

template<typename T>
void World::removeComponent(EntityID entity) {
    // Get the component type ID
    const ComponentTypeID typeID = getComponentTypeID<T>();
    
    // Ensure the entity exists
    if (entity >= m_entities.size()) {
        return;
    }
    
    // Reset the component
    (*m_components[typeID])[entity].reset();
    
    // Clear the component bit
    m_entityMasks[entity].reset(typeID);
}

template<typename T>
bool World::hasComponent(EntityID entity) const {
    // Get the component type ID
    const ComponentTypeID typeID = getComponentTypeID<T>();
    
    // Ensure the entity exists
    if (entity >= m_entities.size()) {
        return false;
    }
    
    // Check the component bit
    return m_entityMasks[entity].test(typeID);
}

template<typename T>
T& World::getComponent(EntityID entity) {
    // Get the component type ID
    const ComponentTypeID typeID = getComponentTypeID<T>();
    
    // Ensure the entity exists and has the component
    if (entity >= m_entities.size() || !m_entityMasks[entity].test(typeID)) {
        throw std::runtime_error("Entity does not have the component");
    }
    
    // Return the component
    return *static_cast<T*>((*m_components[typeID])[entity].get());
}

template<typename... Components>
std::vector<Entity> World::getEntitiesWith() {
    std::vector<Entity> result;
    
    // Create a component mask for the required components
    ComponentMask mask;
    (mask.set(getComponentTypeID<Components>()), ...);
    
    // Find entities with the matching component mask
    for (size_t i = 0; i < m_entities.size(); i++) {
        if ((m_entityMasks[i] & mask) == mask) {
            result.emplace_back(static_cast<EntityID>(i), this);
        }
    }
    
    return result;
}

// Leave only the implementation for the getComponentTypeID() used by the method definitions
// The WebAssembly template instantiations will be added in a future update

} // namespace amamoto