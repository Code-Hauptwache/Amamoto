/**
 * WebGL Renderer for Amamoto Traffic Simulation
 * This module provides GPU-accelerated rendering for road networks and vehicles
 */

class WebGLRenderer {
    constructor(canvas) {
        this.canvas = canvas;
        this.gl = canvas.getContext('webgl2');
        
        if (!this.gl) {
            console.error('WebGL2 not supported. Falling back to WebGL1.');
            this.gl = canvas.getContext('webgl') || canvas.getContext('experimental-webgl');
            if (!this.gl) {
                throw new Error('WebGL not supported in this browser.');
            }
        }
        
        // Initialize GL parameters
        this.gl.clearColor(0.1, 0.1, 0.1, 1.0);
        this.gl.enable(this.gl.DEPTH_TEST);
        this.gl.enable(this.gl.BLEND);
        this.gl.blendFunc(this.gl.SRC_ALPHA, this.gl.ONE_MINUS_SRC_ALPHA);
        
        // Initialize shaders
        this.initShaders();
        
        // Initialize buffers
        this.roadSegmentBuffer = this.gl.createBuffer();
        this.vehicleBuffer = this.gl.createBuffer();
        
        // Camera transformations
        this.cameraOffset = { x: 0, y: 0 };
        this.cameraZoom = 1.0;
        
        // Render statistics
        this.frameCount = 0;
        this.lastFrameTime = 0;
        this.fps = 0;
        
        console.log('WebGL Renderer initialized');
    }
    
    /**
     * Initialize WebGL shaders for rendering roads and vehicles
     */
    initShaders() {
        // Road segment shader
        const roadVertexShaderSource = `
            attribute vec2 aPosition;
            attribute vec4 aColor;
            
            uniform mat3 uTransform;
            
            varying vec4 vColor;
            
            void main() {
                vec3 transformed = uTransform * vec3(aPosition, 1.0);
                gl_Position = vec4(transformed.xy, 0.0, 1.0);
                vColor = aColor;
            }
        `;
        
        const roadFragmentShaderSource = `
            precision mediump float;
            
            varying vec4 vColor;
            
            void main() {
                gl_FragColor = vColor;
            }
        `;
        
        // Vehicle shader with rotation
        const vehicleVertexShaderSource = `
            attribute vec2 aPosition;
            attribute vec2 aOffset;
            attribute float aRotation;
            attribute vec4 aColor;
            
            uniform mat3 uTransform;
            
            varying vec4 vColor;
            
            void main() {
                // Apply rotation
                float s = sin(aRotation);
                float c = cos(aRotation);
                vec2 rotatedPos = vec2(
                    aPosition.x * c - aPosition.y * s,
                    aPosition.x * s + aPosition.y * c
                );
                
                // Apply offset and transform
                vec3 transformed = uTransform * vec3(rotatedPos + aOffset, 1.0);
                gl_Position = vec4(transformed.xy, 0.0, 1.0);
                vColor = aColor;
            }
        `;
        
        const vehicleFragmentShaderSource = `
            precision mediump float;
            
            varying vec4 vColor;
            
            void main() {
                gl_FragColor = vColor;
            }
        `;
        
        // Compile shaders
        this.roadShaderProgram = this.createShaderProgram(roadVertexShaderSource, roadFragmentShaderSource);
        this.vehicleShaderProgram = this.createShaderProgram(vehicleVertexShaderSource, vehicleFragmentShaderSource);
        
        // Get attribute and uniform locations
        this.roadShaderAttributes = {
            position: this.gl.getAttribLocation(this.roadShaderProgram, 'aPosition'),
            color: this.gl.getAttribLocation(this.roadShaderProgram, 'aColor')
        };
        
        this.vehicleShaderAttributes = {
            position: this.gl.getAttribLocation(this.vehicleShaderProgram, 'aPosition'),
            offset: this.gl.getAttribLocation(this.vehicleShaderProgram, 'aOffset'),
            rotation: this.gl.getAttribLocation(this.vehicleShaderProgram, 'aRotation'),
            color: this.gl.getAttribLocation(this.vehicleShaderProgram, 'aColor')
        };
        
        this.roadShaderUniforms = {
            transform: this.gl.getUniformLocation(this.roadShaderProgram, 'uTransform')
        };
        
        this.vehicleShaderUniforms = {
            transform: this.gl.getUniformLocation(this.vehicleShaderProgram, 'uTransform')
        };
    }
    
    /**
     * Helper function to create a shader program from vertex and fragment shader sources
     */
    createShaderProgram(vertexShaderSource, fragmentShaderSource) {
        const vertexShader = this.gl.createShader(this.gl.VERTEX_SHADER);
        this.gl.shaderSource(vertexShader, vertexShaderSource);
        this.gl.compileShader(vertexShader);
        
        if (!this.gl.getShaderParameter(vertexShader, this.gl.COMPILE_STATUS)) {
            console.error('Vertex shader compilation error:', this.gl.getShaderInfoLog(vertexShader));
            this.gl.deleteShader(vertexShader);
            return null;
        }
        
        const fragmentShader = this.gl.createShader(this.gl.FRAGMENT_SHADER);
        this.gl.shaderSource(fragmentShader, fragmentShaderSource);
        this.gl.compileShader(fragmentShader);
        
        if (!this.gl.getShaderParameter(fragmentShader, this.gl.COMPILE_STATUS)) {
            console.error('Fragment shader compilation error:', this.gl.getShaderInfoLog(fragmentShader));
            this.gl.deleteShader(vertexShader);
            this.gl.deleteShader(fragmentShader);
            return null;
        }
        
        const program = this.gl.createProgram();
        this.gl.attachShader(program, vertexShader);
        this.gl.attachShader(program, fragmentShader);
        this.gl.linkProgram(program);
        
        if (!this.gl.getProgramParameter(program, this.gl.LINK_STATUS)) {
            console.error('Shader program linking error:', this.gl.getProgramInfoLog(program));
            this.gl.deleteProgram(program);
            this.gl.deleteShader(vertexShader);
            this.gl.deleteShader(fragmentShader);
            return null;
        }
        
        return program;
    }
    
    /**
     * Update the camera transform matrix based on offset and zoom
     */
    updateCameraTransform() {
        // Convert canvas coordinates to clip space (-1 to 1)
        const canvasToClipSpace = (x, y) => {
            return [
                (x / this.canvas.width) * 2 - 1,
                ((this.canvas.height - y) / this.canvas.height) * 2 - 1
            ];
        };
        
        // Apply camera offset and zoom
        const offsetX = this.cameraOffset.x / (this.canvas.width / 2) * this.cameraZoom;
        const offsetY = this.cameraOffset.y / (this.canvas.height / 2) * this.cameraZoom;
        
        // Create transformation matrix (scale and translate)
        return [
            1 / this.cameraZoom, 0, 0,
            0, 1 / this.cameraZoom, 0,
            -offsetX, -offsetY, 1
        ];
    }
    
    /**
     * Render the road network
     */
    renderRoadNetwork(roadNetwork) {
        const gl = this.gl;
        
        // Use road shader program
        gl.useProgram(this.roadShaderProgram);
        
        // Set camera transform
        const transformMatrix = this.updateCameraTransform();
        gl.uniformMatrix3fv(this.roadShaderUniforms.transform, false, transformMatrix);
        
        // Prepare road segment data
        const vertices = [];
        const roadWidth = 10; // Width in pixels
        
        for (const roadId of roadNetwork.getRoadIds()) {
            const road = roadNetwork.getRoadSegment(roadId);
            const start = road.startPoint;
            const end = road.endPoint;
            
            // Calculate road direction and normal
            const dx = end.x - start.x;
            const dy = end.y - start.y;
            const length = Math.sqrt(dx * dx + dy * dy);
            const dirX = dx / length;
            const dirY = dy / length;
            const normX = -dirY;
            const normY = dirX;
            
            // Calculate the four corners of the road segment
            const halfWidth = roadWidth / 2;
            const x1 = start.x + normX * halfWidth;
            const y1 = start.y + normY * halfWidth;
            const x2 = start.x - normX * halfWidth;
            const y2 = start.y - normY * halfWidth;
            const x3 = end.x - normX * halfWidth;
            const y3 = end.y - normY * halfWidth;
            const x4 = end.x + normX * halfWidth;
            const y4 = end.y + normY * halfWidth;
            
            // Add road segment as two triangles
            // Convert to clip space coordinates
            const [cx1, cy1] = canvasToClipSpace(x1, y1);
            const [cx2, cy2] = canvasToClipSpace(x2, y2);
            const [cx3, cy3] = canvasToClipSpace(x3, y3);
            const [cx4, cy4] = canvasToClipSpace(x4, y4);
            
            // First triangle
            vertices.push(
                cx1, cy1, 0.5, 0.5, 0.5, 1.0, // Position and color (gray)
                cx2, cy2, 0.5, 0.5, 0.5, 1.0,
                cx3, cy3, 0.5, 0.5, 0.5, 1.0
            );
            
            // Second triangle
            vertices.push(
                cx1, cy1, 0.5, 0.5, 0.5, 1.0,
                cx3, cy3, 0.5, 0.5, 0.5, 1.0,
                cx4, cy4, 0.5, 0.5, 0.5, 1.0
            );
        }
        
        // Upload data to GPU
        gl.bindBuffer(gl.ARRAY_BUFFER, this.roadSegmentBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertices), gl.DYNAMIC_DRAW);
        
        // Set up attributes
        const stride = 6 * Float32Array.BYTES_PER_ELEMENT;
        
        gl.enableVertexAttribArray(this.roadShaderAttributes.position);
        gl.vertexAttribPointer(this.roadShaderAttributes.position, 2, gl.FLOAT, false, stride, 0);
        
        gl.enableVertexAttribArray(this.roadShaderAttributes.color);
        gl.vertexAttribPointer(this.roadShaderAttributes.color, 4, gl.FLOAT, false, stride, 2 * Float32Array.BYTES_PER_ELEMENT);
        
        // Draw road segments
        gl.drawArrays(gl.TRIANGLES, 0, vertices.length / 6);
    }
    
    /**
     * Render vehicles using instanced drawing
     */
    renderVehicles(simulation) {
        const gl = this.gl;
        
        // Use vehicle shader program
        gl.useProgram(this.vehicleShaderProgram);
        
        // Set camera transform
        const transformMatrix = this.updateCameraTransform();
        gl.uniformMatrix3fv(this.vehicleShaderUniforms.transform, false, transformMatrix);
        
        // Get vehicle count
        const vehicleCount = simulation.getVehicleCount();
        if (vehicleCount === 0) return;
        
        // Prepare vehicle data
        const vehicleData = [];
        
        // For each vehicle, get position, rotation, and color
        for (let i = 0; i < vehicleCount; i++) {
            const pos = simulation.getVehiclePosition(i);
            const vel = simulation.getVehicleVelocity(i);
            
            // Calculate rotation from velocity
            const rotation = Math.atan2(vel.y, vel.x);
            
            // Add to data array (position.x, position.y, rotation, r, g, b, a)
            vehicleData.push(
                pos.x, pos.y, rotation,
                0.2, 0.6, 0.8, 1.0 // Vehicle color (blue)
            );
        }
        
        // Upload data to GPU
        gl.bindBuffer(gl.ARRAY_BUFFER, this.vehicleBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vehicleData), gl.DYNAMIC_DRAW);
        
        // Create vehicle shape (rectangle)
        const vehicleWidth = 4;  // Width in pixels
        const vehicleLength = 8; // Length in pixels
        
        const vehicleShape = [
            // Front triangle
            0, -vehicleLength/2,
            vehicleWidth/2, vehicleLength/2,
            -vehicleWidth/2, vehicleLength/2,
            
            // Back rectangle
            vehicleWidth/2, vehicleLength/2,
            -vehicleWidth/2, vehicleLength/2,
            vehicleWidth/2, 0,
            
            -vehicleWidth/2, vehicleLength/2,
            -vehicleWidth/2, 0,
            vehicleWidth/2, 0
        ];
        
        // For each vehicle, draw the shape at the vehicle's position and rotation
        for (let i = 0; i < vehicleCount; i++) {
            // Set vertex attributes for this vehicle
            const offset = i * 7; // Each vehicle has 7 values (x, y, rotation, r, g, b, a)
            
            // Draw vehicle shape
            for (let j = 0; j < vehicleShape.length; j += 2) {
                const x = vehicleShape[j];
                const y = vehicleShape[j + 1];
                
                // TODO: Apply rotation and offset
            }
        }
        
        // Draw vehicles
        gl.drawArrays(gl.TRIANGLES, 0, vehicleCount * 9); // 9 vertices per vehicle (3 triangles)
    }
    
    /**
     * Render the scene with roads and vehicles
     */
    render(roadNetwork, simulation, timestamp) {
        const gl = this.gl;
        
        // Calculate FPS
        if (timestamp - this.lastFrameTime >= 1000) {
            this.fps = this.frameCount;
            this.frameCount = 0;
            this.lastFrameTime = timestamp;
        }
        this.frameCount++;
        
        // Clear the canvas
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
        
        // Render road network
        if (roadNetwork) {
            this.renderRoadNetwork(roadNetwork);
        }
        
        // Render vehicles
        if (simulation) {
            this.renderVehicles(simulation);
        }
        
        return this.fps;
    }
    
    /**
     * Set the camera offset (for panning)
     */
    setCameraOffset(x, y) {
        this.cameraOffset.x = x;
        this.cameraOffset.y = y;
    }
    
    /**
     * Set the camera zoom level
     */
    setCameraZoom(zoom) {
        this.cameraZoom = Math.max(0.1, Math.min(10, zoom));
    }
    
    /**
     * Resize the renderer to match canvas dimensions
     */
    resize() {
        this.gl.viewport(0, 0, this.canvas.width, this.canvas.height);
    }
    
    /**
     * Helper function to convert canvas coordinates to clip space
     */
    canvasToClipSpace(x, y) {
        return [
            (x / this.canvas.width) * 2 - 1,
            ((this.canvas.height - y) / this.canvas.height) * 2 - 1
        ];
    }
}

export { WebGLRenderer };