// Simulation module that integrates with the road network
import { RoadNetwork } from './road-network.js';
import { TrafficIntegration } from './traffic-integration.js';
import { loadWasmModule } from './wasm-integration.js';

// Performance tracking
let lastFrameTime = 0;
let frameCount = 0;
let lastFpsUpdate = 0;
let currentFps = 0;

// Simulation state
let simulationRunning = false;
let simulation = null;
let wasmMemory = null;
let roadNetwork = null;
let trafficIntegration = null;

// Canvas and rendering context
let canvas = null;
let ctx = null;

// UI elements - will be initialized when document is loaded
let statusMessage = null;
let vehicleCountElement = null;
let fpsElement = null;
let memoryUsageElement = null;
let startButton = null;
let stopButton = null;
let addVehicleButton = null;
let add100VehiclesButton = null;
let clearButton = null;
let keepInBoundsCb = null;
let simulationStatus = null;

// Initialize canvas size
function initCanvas() {
    if (!canvas) return;
    canvas.width = canvas.clientWidth;
    canvas.height = canvas.clientHeight;
}
// Get road network from editor
function initializeWithRoadNetwork(network) {
    roadNetwork = network;
}

// Initialize the simulation
async function initSimulation() {
    // Initialize UI elements if they haven't been already
    if (!simulationStatus) {
        // Get UI elements
        canvas = document.getElementById('simulation-canvas');
        ctx = canvas.getContext('2d');
        statusMessage = document.getElementById('simulation-status');
        simulationStatus = document.getElementById('simulation-status');
        vehicleCountElement = document.getElementById('vehicle-count');
        fpsElement = document.getElementById('fps');
        memoryUsageElement = document.getElementById('memory-usage');
        startButton = document.getElementById('start-btn');
        stopButton = document.getElementById('stop-btn');
        addVehicleButton = document.getElementById('add-vehicle-btn');
        add100VehiclesButton = document.getElementById('add-100-vehicles-btn');
        clearButton = document.getElementById('clear-btn');
        keepInBoundsCb = document.getElementById('keep-in-bounds-cb');
        
        // Initialize canvas
        initCanvas();
    }
    simulationStatus.textContent = 'Loading simulation module (WebAssembly implementation)...';
    
    try {
        // Load the WebAssembly simulation using the integration module
        const { simulation: wasmSimulation, memory } = await loadWasmModule();
        
        // Set the simulation and memory
        simulation = wasmSimulation;
        wasmMemory = memory;
        wasmMemory = memory;
        
        // If we have a road network, create traffic integration
        if (roadNetwork) {
            trafficIntegration = new TrafficIntegration(roadNetwork, simulation);
        }
        
        // Update UI
        simulationStatus.textContent = 'Simulation module loaded successfully (WebAssembly implementation)';
        simulationStatus.className = 'status';
        
        // Enable controls
        addVehicleButton.disabled = false;
        add100VehiclesButton.disabled = false;
        clearButton.disabled = false;
        startButton.disabled = false;
        
        // Add event listeners
        addVehicleButton.addEventListener('click', addRandomVehicle);
        add100VehiclesButton.addEventListener('click', add100RandomVehicles);
        clearButton.addEventListener('click', clearSimulation);
        startButton.addEventListener('click', startSimulation);
        stopButton.addEventListener('click', stopSimulation);
        
        // Add initial vehicles based on context
        if (trafficIntegration && roadNetwork.roadSegments.size > 0) {
            // If we have a road network with roads, add vehicles to it
            trafficIntegration.addRandomVehicles(100);
        } else {
            // Otherwise add random vehicles in the open space
            for (let i = 0; i < 100; i++) {
                addRandomVehicle();
            }
        }
        
        // Initial render
        render();
        
    } catch (error) {
        console.error('Failed to load simulation module:', error);
        simulationStatus.textContent = 'Error: Failed to load simulation module. ' + error.message;
        simulationStatus.className = 'status error';
    }
}

// Add a random vehicle to the simulation
function addRandomVehicle() {
    if (!simulation) return -1;
    
    // If we have road network integration, add vehicle to network
    if (trafficIntegration && roadNetwork && roadNetwork.roadSegments.size > 0) {
        const id = trafficIntegration.addRandomVehicleOnNetwork();
        if (id >= 0) {
            updateVehicleCount();
            return id;
        }
    }
    
    // Otherwise add a random vehicle in open space
    const x = Math.random() * canvas.width;
    const y = Math.random() * canvas.height;
    
    // Random velocity (-50 to 50 pixels per second)
    const vx = (Math.random() - 0.5) * 100;
    const vy = (Math.random() - 0.5) * 100;
    
    // Add to simulation
    const id = simulation.createVehicle(x, y, vx, vy);
    
    // Update count
    updateVehicleCount();
    
    return id;
}

// Add 100 random vehicles to the simulation
function add100RandomVehicles() {
    if (!simulation) return;
    
    // If we have road network integration, add vehicles to network
    if (trafficIntegration && roadNetwork && roadNetwork.roadSegments.size > 0) {
        trafficIntegration.addRandomVehicles(100);
    } else {
        // Otherwise add random vehicles in open space
        for (let i = 0; i < 100; i++) {
            addRandomVehicle();
        }
    }
    
    // Update count and render
    updateVehicleCount();
    render();
}

// Clear all vehicles
function clearSimulation() {
    if (!simulation) return;
    
    simulation.clear();
    
    // Clear path maps if using traffic integration
    if (trafficIntegration) {
        trafficIntegration = new TrafficIntegration(roadNetwork, simulation);
    }
    
    updateVehicleCount();
    render();
}

// Start the simulation
function startSimulation() {
    if (!simulation || simulationRunning) return;
    
    simulationRunning = true;
    startButton.disabled = true;
    stopButton.disabled = false;
    
    simulationStatus.textContent = 'Simulation running...';
    
    // Start animation loop
    lastFrameTime = performance.now();
    requestAnimationFrame(simulationLoop);
}

// Stop the simulation
function stopSimulation() {
    simulationRunning = false;
    startButton.disabled = false;
    stopButton.disabled = true;
}

// Main simulation loop
function simulationLoop(timestamp) {
    if (!simulationRunning) return;
    
    // Calculate delta time in seconds
    const deltaTime = (timestamp - lastFrameTime) / 1000;
    lastFrameTime = timestamp;
    
    // Update traffic integration if available
    if (trafficIntegration && roadNetwork && roadNetwork.roadSegments.size > 0) {
        trafficIntegration.updateVehicles(deltaTime);
    }
    
    // Update simulation - pass bounds if keep-in-bounds is checked
    if (keepInBoundsCb.checked) {
        // Pass canvas dimensions as bounds
        const bounds = {
            width: canvas.width,
            height: canvas.height
        };
        simulation.update(deltaTime, bounds);
    } else {
        simulation.update(deltaTime);
    }
    
    // Render road network first if available
    if (roadNetwork) {
        roadNetwork.render(ctx);
    }
    
    // Render vehicles
    render();
    
    // Update FPS counter
    frameCount++;
    if (timestamp - lastFpsUpdate >= 1000) {
        currentFps = Math.round(frameCount * 1000 / (timestamp - lastFpsUpdate));
        fpsElement.textContent = currentFps;
        frameCount = 0;
        lastFpsUpdate = timestamp;
        
        // Update memory usage (convert bytes to MB)
        if (wasmMemory) {
            // For our JavaScript implementation, update the simulated memory size based on vehicle count
            if (simulation && typeof simulation.getVehicleCount === 'function' &&
                wasmMemory.updateMemorySize && typeof wasmMemory.updateMemorySize === 'function') {
                wasmMemory.updateMemorySize(simulation.getVehicleCount());
            }
            const memoryUsageMB = Math.round(wasmMemory.byteLength / (1024 * 1024) * 100) / 100;
            memoryUsageElement.textContent = memoryUsageMB;
        }
    }
    
    // Continue loop
    requestAnimationFrame(simulationLoop);
}

// Render the simulation state
function render() {
    if (!simulation) return;
    
    // Clear canvas
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    // Render road network first if available
    if (roadNetwork) {
        roadNetwork.render(ctx);
    }
    
    // Get vehicle count
    const vehicleCount = simulation.getVehicleCount();
    
    // Draw each vehicle
    for (let i = 0; i < vehicleCount; i++) {
        const position = simulation.getVehiclePosition(i);
        const velocity = simulation.getVehicleVelocity(i);
        
        // Calculate angle based on velocity for proper orientation
        const angle = Math.atan2(velocity.y, velocity.x);
        
        // Draw vehicle as a car shape
        ctx.save();
        ctx.translate(position.x, position.y);
        ctx.rotate(angle);
        
        // Car body
        ctx.beginPath();
        ctx.rect(-10, -5, 20, 10);
        ctx.fillStyle = `hsl(${(i * 30) % 360}, 70%, 60%)`;
        ctx.fill();
        ctx.stroke();
        
        // Restore canvas state
        ctx.restore();
    }
}

// Update the vehicle count display
function updateVehicleCount() {
    if (!simulation) return;
    vehicleCountElement.textContent = simulation.getVehicleCount();
}

// Handle window resize
window.addEventListener('resize', () => {
    initCanvas();
    render();
});

// Initialize
document.addEventListener('DOMContentLoaded', () => {
    initCanvas();
    initSimulation();
});

// Check if the module failed to load after a timeout
setTimeout(() => {
    if (!simulation) {
        statusMessage.textContent = 'Warning: WebAssembly module taking longer than expected to load. If it continues to fail, check console for errors.';
        statusMessage.className = 'status error';
    }
}, 5000);

// Export functions needed by other modules
export { initializeWithRoadNetwork, initSimulation };