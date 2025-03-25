#include "WasmECS.h"

namespace amamoto {

WasmWorld::WasmWorld() : m_entityCount(0) {
    // Reserve space for initial entities
    m_entities.reserve(1000);
    m_entityMasks.reserve(1000);
    m_componentPools.reserve(MAX_COMPONENTS);
}

WasmWorld::~WasmWorld() {
    // Clear all entities and components
    m_entities.clear();
    m_entityMasks.clear();
    m_freeEntities.clear();
    m_componentPools.clear();
    m_systems.clear();
}

WasmEntity WasmWorld::createEntity() {
    WasmEntityID id;
    
    // Reuse an ID from the free list if available
    if (!m_freeEntities.empty()) {
        id = m_freeEntities.back();
        m_freeEntities.pop_back();
        
        // Reset the entity's component mask
        m_entityMasks[id].reset();
    } else {
        // Create a new ID
        id = static_cast<WasmEntityID>(m_entities.size());
        m_entities.push_back(id);
        m_entityMasks.push_back(WasmComponentMask());
    }
    
    m_entityCount++;
    return WasmEntity(id, this);
}

void WasmWorld::destroyEntity(WasmEntity entity) {
    const WasmEntityID id = entity.getID();
    
    if (!entityExists(id)) {
        return;
    }
    
    // Remove all components
    for (WasmComponentID componentId = 0; componentId < m_componentPools.size(); componentId++) {
        if (m_entityMasks[id].test(componentId) && m_componentPools[componentId]) {
            m_componentPools[componentId]->removeComponent(id);
        }
    }
    
    // Clear the entity's component mask
    m_entityMasks[id].reset();
    
    // Add the ID to the free list
    m_freeEntities.push_back(id);
    m_entityCount--;
}

bool WasmWorld::entityExists(WasmEntityID id) const {
    return id < m_entities.size() && m_entityMasks[id].any();
}

void WasmWorld::update(float dt) {
    // Run all registered systems
    for (auto& system : m_systems) {
        system(*this, dt);
    }
}

void WasmWorld::registerSystem(std::function<void(WasmWorld&, float)> system) {
    m_systems.push_back(system);
}

} // namespace amamoto