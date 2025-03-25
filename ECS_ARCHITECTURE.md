# Entity Component System (ECS) Architecture in Amamoto

This document provides an overview of the Entity Component System (ECS) architecture implemented in the Amamoto traffic simulation project.

## Overview

The Amamoto traffic simulation now uses an Entity Component System (ECS) architecture for its WebAssembly implementation. ECS is a software architectural pattern that follows composition over inheritance and separates the data (components) from behavior (systems).

## Core Concepts

1. **Entities**: Simple identifiers that represent objects in the simulation (vehicles, traffic lights, etc.)
2. **Components**: Pure data containers that are attached to entities (position, velocity, etc.)
3. **Systems**: Logic that operates on entities with specific sets of components

## Benefits

- **Performance**: Better cache utilization through data-oriented design
- **Flexibility**: Easy to add/remove components without changing existing code
- **Modularity**: Systems operate independently on component data
- **Scalability**: Efficient handling of thousands of entities
- **WebAssembly compatibility**: Addresses template instantiation issues

## Implementation Details

### Entity Management

```cpp
// Entity is essentially just an ID with a reference to its World
class WasmEntity {
    WasmEntityID m_id;
    WasmWorld* m_world;
    bool m_valid;
};

// World manages entities and their component assignments
class WasmWorld {
    std::vector<WasmEntityID> m_entities;
    std::vector<WasmComponentMask> m_entityMasks;
    std::vector<WasmEntityID> m_freeEntities;
    std::vector<std::unique_ptr<WasmComponentPool>> m_componentPools;
};
```

### Component System

Components are simple data structures that inherit from the base `WasmComponent` class:

```cpp
class WasmComponent {
    virtual ~WasmComponent() = default;
    virtual const char* getTypeName() const = 0;
};

class WasmTransformComponent : public WasmComponent {
    WasmVector2D position;
    WasmVector2D velocity;
    float rotation;
};
```

Key components include:
- `TransformComponent`: Position, velocity, and rotation
- `VehicleComponent`: Vehicle-specific properties
- `PathFollowingComponent`: Path navigation
- `BoundsComponent`: Simulation boundaries
- `CollisionComponent`: Collision detection
- `RenderableComponent`: Visual representation
- `TrafficSignalComponent`: Traffic light behavior
- `SelectableComponent`: UI selection

### Systems

Systems implement the behavior and logic of the simulation:

```cpp
// System function signature
using WasmSystemFunction = std::function<void(WasmWorld&, float)>;

// Systems registered with the World
void WasmWorld::registerSystem(WasmSystemFunction system) {
    m_systems.push_back(system);
}

// Update calls all registered systems
void WasmWorld::update(float dt) {
    for (auto& system : m_systems) {
        system(*this, dt);
    }
}
```

Core systems include:
- Movement system
- Bounds system
- Path following system
- Collision system
- Traffic signal system

### Component Queries

ECS provides efficient component queries to locate entities with specific components:

```cpp
// Get all entities with the specified components
template<typename... Components>
std::vector<WasmEntity> WasmWorld::getEntitiesWith() {
    std::vector<WasmEntity> entities;
    WasmComponentMask requiredMask;
    (requiredMask.set(WasmComponentRegistry::getComponentID<Components>()), ...);
    
    for (size_t i = 0; i < m_entityMasks.size(); i++) {
        if ((m_entityMasks[i] & requiredMask) == requiredMask) {
            entities.emplace_back(static_cast<WasmEntityID>(i), this);
        }
    }
    
    return entities;
}
```

## WebAssembly Integration

The ECS architecture is exported to JavaScript through Emscripten bindings:

```cpp
EMSCRIPTEN_BINDINGS(amamoto_ecs) {
    // Bind ECS components and systems
    class_<amamoto::WasmComponent>("Component");
    class_<amamoto::WasmTransformComponent, base<amamoto::WasmComponent>>("TransformComponent");
    class_<amamoto::WasmTrafficSimulation>("TrafficSimulation");
}
```

## Building the ECS-based Implementation

1. Use the provided build scripts:
   ```
   # Windows
   .\build-wasm-ecs.bat
   
   # Unix/macOS
   chmod +x build-wasm-ecs.sh
   ./build-wasm-ecs.sh
   ```

2. Include the generated WebAssembly module:
   ```javascript
   import { loadWasmECSModule } from './wasm-ecs-integration.js';
   
   async function init() {
     const { simulation, roadNetwork } = await loadWasmECSModule();
     // Use the simulation and road network
   }
   ```

## Future Improvements

1. **Multi-threaded Systems**:
   - Implement parallel system execution using Web Workers
   - Process component data in parallel based on dependencies

2. **Advanced Component Features**:
   - Component inheritance for specialized behaviors
   - Component references for complex relationships

3. **Dynamic Component Registration**:
   - Runtime component type registration
   - Component serialization for save/load functionality

4. **Performance Optimizations**:
   - Archetype-based component storage for cache efficiency
   - Batch processing for WebGL integration

## Compatibility Notes

- Requires C++17 support for compilation
- Emscripten 2.0.0+ required for proper WebAssembly bindings
- Modern browser with WebAssembly support needed for execution