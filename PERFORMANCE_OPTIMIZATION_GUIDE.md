# Performance Optimization Guide for Amamoto

This guide outlines strategies for optimizing the performance of the Amamoto traffic simulation after the WebAssembly migration.

## Current Performance Profile

The JavaScript implementation provides a baseline for comparison:
- Handles a limited number of vehicles (100-1000) at acceptable framerates
- Simple physics and collision detection
- Basic road network rendering

The WebAssembly implementation should immediately improve performance, but further optimizations will be necessary for large-scale simulations.

## Optimization Strategies

### 1. Memory Management

#### Entity Pooling
- Pre-allocate memory for entities to reduce allocation overhead
- Reuse entity slots when vehicles are removed
- Implement a chunked memory allocation strategy

```cpp
// Example of entity pooling in C++
class EntityPool {
private:
    std::vector<Entity> entities;
    std::vector<size_t> freeIndices;
    
public:
    EntityPool(size_t initialCapacity) {
        entities.reserve(initialCapacity);
        // Initialize free indices
    }
    
    size_t createEntity() {
        if (!freeIndices.empty()) {
            // Reuse existing slot
            size_t index = freeIndices.back();
            freeIndices.pop_back();
            entities[index] = Entity();
            return index;
        } else {
            // Create new entity
            entities.push_back(Entity());
            return entities.size() - 1;
        }
    }
    
    void destroyEntity(size_t index) {
        freeIndices.push_back(index);
    }
};
```

#### Minimize JavaScript-WebAssembly Boundary Crossing
- Pass bulk data between JavaScript and WebAssembly
- Use shared memory where possible
- Cache results to avoid redundant transfers

### 2. Computation Optimization

#### Spatial Partitioning
- Implement grid-based spatial partitioning
- Only process interactions between nearby entities
- Update partition assignments only when entities move significantly

```cpp
// Example spatial partitioning approach
class SpatialGrid {
private:
    std::vector<std::vector<size_t>> cells;
    float cellSize;
    int gridWidth, gridHeight;
    
public:
    void addEntity(size_t entityId, const Vector2& position) {
        int cellX = static_cast<int>(position.x / cellSize);
        int cellY = static_cast<int>(position.y / cellSize);
        
        if (isValidCell(cellX, cellY)) {
            cells[cellY * gridWidth + cellX].push_back(entityId);
        }
    }
    
    std::vector<size_t> getNearbyEntities(const Vector2& position, float radius) {
        // Get all entities in cells that overlap with the query radius
        // ...
    }
};
```

#### Vehicle Update Batching
- Process vehicles in batches to improve cache locality
- Group vehicles by road segment or spatial location
- Parallelize updates across multiple threads using Web Workers

#### Adaptive Time Steps
- Use variable time steps based on simulation load
- Implement physics sub-stepping for stability
- Prioritize updates for visible areas

### 3. Rendering Optimization

#### WebGL Integration
- Move rendering to WebGL for GPU acceleration
- Use instanced rendering for vehicles
- Implement custom shaders for road network visualization

```javascript
// Example of WebGL instanced rendering setup
function setupInstancedRendering(gl, vehicleCount) {
    // Create buffer for instance data
    const instanceBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, instanceBuffer);
    
    // Allocate buffer large enough for all instances
    const instanceData = new Float32Array(vehicleCount * 6); // position(2) + rotation(1) + color(3)
    gl.bufferData(gl.ARRAY_BUFFER, instanceData, gl.DYNAMIC_DRAW);
    
    // Set up vertex attributes for instancing
    // ...
    
    // Enable instanced arrays extension if needed
    const ext = gl.getExtension('ANGLE_instanced_arrays');
    
    // Return objects needed for rendering
    return {
        instanceBuffer,
        instanceData,
        instanceExtension: ext
    };
}
```

#### Level of Detail Rendering
- Simplify rendering for distant objects
- Fade out vehicles that are far from the camera
- Use lower-resolution models for distant entities

#### Culling Techniques
- Implement frustum culling to skip rendering off-screen elements
- Use occlusion culling for complex scenes
- Cull small or distant objects based on zoom level

### 4. Road Network Optimization

#### Path Caching
- Cache commonly used paths
- Pre-compute paths for regular routes
- Implement incremental path updates when the network changes

```javascript
// Path caching example
class PathCache {
    constructor(maxCacheSize = 1000) {
        this.cache = new Map();
        this.maxSize = maxCacheSize;
    }
    
    getPathKey(startPos, endPos) {
        return `${Math.round(startPos.x)},${Math.round(startPos.y)}_${Math.round(endPos.x)},${Math.round(endPos.y)}`;
    }
    
    getPath(startPos, endPos) {
        const key = this.getPathKey(startPos, endPos);
        return this.cache.get(key);
    }
    
    storePath(startPos, endPos, path) {
        if (this.cache.size >= this.maxSize) {
            // Evict oldest entry
            const oldestKey = this.cache.keys().next().value;
            this.cache.delete(oldestKey);
        }
        
        const key = this.getPathKey(startPos, endPos);
        this.cache.set(key, path);
    }
}
```

#### Hierarchical Road Representation
- Use a multi-level representation of the road network
- For high-level pathfinding, consider only major roads
- Refine paths using detailed road information when needed

### 5. Parallel Processing with Web Workers

#### Worker Task Distribution
- Divide simulation work across multiple Web Workers
- Assign spatial regions to different workers
- Synchronize at boundaries with message passing

```javascript
// Web Worker for simulation example
// main.js
function initializeWorkers(workerCount) {
    const workers = [];
    for (let i = 0; i < workerCount; i++) {
        const worker = new Worker('simulation-worker.js');
        worker.postMessage({
            type: 'init',
            regionId: i,
            regionCount: workerCount
        });
        workers.push(worker);
    }
    return workers;
}

// simulation-worker.js
self.onmessage = function(e) {
    if (e.data.type === 'init') {
        // Initialize worker with region information
    } else if (e.data.type === 'update') {
        // Run simulation step for this region
        // ...
        self.postMessage({
            type: 'updateComplete',
            entities: updatedEntities
        });
    }
};
```

#### Shared Memory Between Workers
- Use SharedArrayBuffer for direct memory access
- Implement proper synchronization with Atomics
- Minimize message-passing overhead

#### Worker Coordination
- Implement barrier synchronization for simulation steps
- Use a main thread coordinator for global state
- Handle load balancing dynamically

## Benchmarking and Profiling

### Performance Testing Framework
- Create automated tests for measuring FPS with varying entity counts
- Track memory usage patterns
- Compare performance across different optimization strategies

### Browser Profiling Tools
- Use Chrome DevTools Performance tab for detailed analysis
- Identify hot spots and optimization opportunities
- Monitor memory allocation and garbage collection

### WebAssembly-Specific Profiling
- Use Emscripten's built-in profiling options
- Analyze WASM call stacks and execution time
- Profile memory transfer between JavaScript and WebAssembly

## Implementation Priority

For maximum impact, implement optimizations in this order:

1. **Memory Management** - Provides immediate benefits with relatively low implementation effort
2. **Spatial Partitioning** - Critical for scaling to larger simulations
3. **WebGL Rendering** - Significant performance boost for visualization
4. **Path Caching** - Important for road network scaling
5. **Web Workers** - Enable multi-core utilization for very large simulations

Each optimization should be benchmarked before and after implementation to quantify benefits and identify potential regressions.