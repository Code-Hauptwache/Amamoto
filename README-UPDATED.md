# Amamoto Traffic Simulation

A high-performance traffic simulation platform with road network editor and vehicle simulation, supporting both JavaScript and WebAssembly implementations.

![Amamoto Logo](docs/amamoto-logo.png)

## Quick Start

1. **Run the MVP**
   ```
   cd src/web/public
   python -m http.server 8082
   ```
   Then open http://localhost:8082/mvp.html

2. **Work with the road editor**
   - Create roads using the "Create Road" tool
   - Connect roads with intersections
   - Save your network design
   
3. **Run the simulation**
   - Switch to the Simulation tab
   - Add vehicles to your road network
   - Start the simulation and observe traffic flow

## Project Structure

- `src/web/public/` - Web frontend files
  - `mvp.html` - MVP interface with road editor and simulation
  - `road-editor.js` - Road network editing functionality
  - `road-network.js` - Road network data structures
  - `traffic-integration.js` - Integration between roads and vehicles
  - `simulation.js` - Simulation controller
  - `js-simulation.js` - JavaScript implementation of simulation
  - `wasm-integration.js` - Integration for WebAssembly module (when built)

- `src/simulation/` - C++ simulation engine
  - `core/` - Core entity-component system
  - `TrafficSimulation.cpp/h` - Main simulation implementation
  - `wasm_bindings.cpp` - WebAssembly bindings

## Documentation

- [Architecture Validation](ARCHITECTURE_VALIDATION.md) - Validation of the core architecture
- [Development Roadmap](DEVELOPMENT_ROADMAP.md) - Planned future development
- [C++ Simulation](src/simulation/README.md) - Overview of C++ simulation engine
- [WASM Migration Guide](WASM_MIGRATION_GUIDE.md) - Guide for migrating to WebAssembly
- [Performance Optimization Guide](PERFORMANCE_OPTIMIZATION_GUIDE.md) - Performance improvement strategies

## WebAssembly Compilation

To compile the C++ code to WebAssembly:

### Windows
```
.\setup-wasm-build.bat
```

### Unix/macOS
```
chmod +x setup-wasm-build.sh
./setup-wasm-build.sh
```

This will setup Emscripten and compile the C++ code to WebAssembly. After compilation, the WebAssembly module will be automatically copied to the web directory.

## Development Process

1. **MVP JavaScript Implementation**
   - Initial implementation in JavaScript
   - Road network editor and vehicle simulation
   - Validates core architecture

2. **C++ Implementation**
   - More efficient entity-component system
   - Better performance for large simulations
   - Compiled to WebAssembly for browser execution

3. **Advanced Features**
   - GPU acceleration using WebGL
   - Multi-threading with Web Workers
   - Advanced traffic rules and AI

## Performance Considerations

The JavaScript implementation provides a good development experience and works well for small to medium simulations. For larger simulations (thousands of vehicles), the WebAssembly implementation provides better performance.

See the [Performance Optimization Guide](PERFORMANCE_OPTIMIZATION_GUIDE.md) for detailed strategies on improving simulation performance.

## Contributing

1. Review the [Architecture Validation](ARCHITECTURE_VALIDATION.md) document to understand the design
2. Check the [Development Roadmap](DEVELOPMENT_ROADMAP.md) for planned features
3. Make your changes following the established architecture
4. Ensure both JavaScript and WebAssembly implementations work correctly
5. Submit a pull request with a clear description of the changes

## License

[MIT License](LICENSE)