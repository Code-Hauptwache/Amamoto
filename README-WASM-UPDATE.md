# Amamoto Traffic Simulation with WebAssembly

The Amamoto traffic simulation platform now includes a WebAssembly implementation for improved performance.

## WebAssembly Implementation

The project now includes two implementations:

1. **JavaScript Implementation** - The original implementation that works in all browsers.
2. **WebAssembly Implementation** - A high-performance implementation that compiles C++ to WebAssembly.

The application automatically uses the WebAssembly implementation when available.

## Running the Simulation

The simulation can be run with the following command:

```
cd src/web/public
python -m http.server 8082
```

Then open http://localhost:8082/mvp.html in your browser.

## WebAssembly Build Instructions

### Prerequisites

- Emscripten SDK (installed automatically by the setup script)
- CMake 3.10+
- A C++17 compatible compiler

### Building the WebAssembly Module

#### Windows

```
.\setup-wasm-build.bat   # First time setup to install Emscripten
.\build-wasm-simple.bat  # Build the WebAssembly module
```

#### Unix/macOS

```
chmod +x setup-wasm-build.sh
./setup-wasm-build.sh    # First time setup to install Emscripten

chmod +x build-wasm-simple.sh
./build-wasm-simple.sh   # Build the WebAssembly module
```

## Implementation Details

The WebAssembly implementation uses a simplified version of the traffic simulation architecture, optimized for compiled code. Key features include:

- Non-templated architecture to avoid C++ template instantiation issues
- Direct compilation of C++ to WebAssembly using Emscripten
- JavaScript bindings using Embind
- Compatible API with the JavaScript implementation

## Performance Benefits

The WebAssembly implementation provides several advantages:

1. **Faster Vehicle Updates**: Vehicle positions and velocities are updated more efficiently
2. **Lower Memory Usage**: More efficient memory management
3. **Improved Collision Detection**: Better performance for proximity calculations
4. **Larger Simulations**: Can handle more vehicles with better frame rates

## Future Improvements

Future versions will expand the WebAssembly implementation with:

1. Complete road network implementation in WebAssembly
2. WebGL integration for rendering
3. Multi-threading support using Web Workers
4. Full Entity Component System in WebAssembly