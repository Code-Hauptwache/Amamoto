// Road Editor UI for creating and editing road networks
import { Vector2D, RoadNetwork } from './road-network.js';

// Road editor modes
const EditorMode = {
    SELECT: 'select',
    CREATE_ROAD: 'create_road',
    CREATE_INTERSECTION: 'create_intersection',
    DELETE: 'delete'
};

// Road editor class
class RoadEditor {
    constructor(canvas, onNetworkUpdate = null) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.roadNetwork = new RoadNetwork();
        this.mode = EditorMode.SELECT;
        this.selectedElement = null;
        this.dragStart = null;
        this.dragEnd = null;
        this.isDragging = false;
        this.snapDistance = 20; // Distance in pixels for snapping
        this.showGrid = true;
        this.gridSize = 20;
        this.onNetworkUpdate = onNetworkUpdate; // Callback when network is updated
        
        // Set up event listeners
        this.setupEventListeners();
    }
    
    // Set up event listeners for the canvas
    setupEventListeners() {
        this.canvas.addEventListener('mousedown', this.handleMouseDown.bind(this));
        this.canvas.addEventListener('mousemove', this.handleMouseMove.bind(this));
        this.canvas.addEventListener('mouseup', this.handleMouseUp.bind(this));
        this.canvas.addEventListener('click', this.handleClick.bind(this));
    }
    
    // Set the editor mode
    setMode(mode) {
        this.mode = mode;
        this.selectedElement = null;
        this.dragStart = null;
        this.dragEnd = null;
        this.isDragging = false;
    }
    
    // Toggle grid visibility
    toggleGrid() {
        this.showGrid = !this.showGrid;
        this.render();
    }
    
    // Set grid size
    setGridSize(size) {
        this.gridSize = size;
        if (this.showGrid) {
            this.render();
        }
    }
    
    // Handle mouse down event
    handleMouseDown(event) {
        const rect = this.canvas.getBoundingClientRect();
        const x = event.clientX - rect.left;
        const y = event.clientY - rect.top;
        
        this.dragStart = new Vector2D(x, y);
        
        switch (this.mode) {
            case EditorMode.SELECT:
                // Try to select an element
                const intersection = this.roadNetwork.findNearestIntersection(x, y);
                const road = this.roadNetwork.findNearestRoadSegment(x, y);
                
                if (intersection && (!road || intersection.distance < road.distance)) {
                    this.selectedElement = { type: 'intersection', id: intersection.id };
                } else if (road) {
                    this.selectedElement = { type: 'road', id: road.id };
                } else {
                    this.selectedElement = null;
                }
                break;
                
            case EditorMode.CREATE_ROAD:
                // Snap to nearby intersections or road endpoints
                const snapped = this.snapToNearbyElement(x, y);
                if (snapped) {
                    this.dragStart = snapped;
                }
                
                this.isDragging = true;
                break;
                
            case EditorMode.DELETE:
                // Find and delete the nearest element
                const nearestIntersection = this.roadNetwork.findNearestIntersection(x, y);
                const nearestRoad = this.roadNetwork.findNearestRoadSegment(x, y);
                
                if (nearestIntersection && (!nearestRoad || nearestIntersection.distance < nearestRoad.distance)) {
                    // TODO: Implement intersection deletion
                } else if (nearestRoad) {
                    this.roadNetwork.roadSegments.delete(nearestRoad.id);
                    if (this.onNetworkUpdate) this.onNetworkUpdate();
                }
                break;
        }
        
        this.render();
    }
    
    // Handle mouse move event
    handleMouseMove(event) {
        const rect = this.canvas.getBoundingClientRect();
        const x = event.clientX - rect.left;
        const y = event.clientY - rect.top;
        
        if (this.isDragging && this.mode === EditorMode.CREATE_ROAD) {
            // Snap endpoint to nearby elements if close
            const snapped = this.snapToNearbyElement(x, y);
            this.dragEnd = snapped || new Vector2D(x, y);
            this.render();
        }
    }
    
    // Handle mouse up event
    handleMouseUp(event) {
        const rect = this.canvas.getBoundingClientRect();
        const x = event.clientX - rect.left;
        const y = event.clientY - rect.top;
        
        if (this.isDragging && this.mode === EditorMode.CREATE_ROAD && this.dragStart) {
            // Snap endpoint to nearby elements if close
            const snapped = this.snapToNearbyElement(x, y);
            this.dragEnd = snapped || new Vector2D(x, y);
            
            // Create road if start and end are different points
            if (this.dragStart.distanceTo(this.dragEnd) > 10) {
                const roadId = this.roadNetwork.createRoadSegment(
                    this.dragStart.x, this.dragStart.y,
                    this.dragEnd.x, this.dragEnd.y
                );
                
                // If both ends snapped to intersections, connect them
                // TODO: Implement this once we have intersection-road connection logic
                
                if (this.onNetworkUpdate) this.onNetworkUpdate();
            }
        }
        
        this.isDragging = false;
        this.render();
    }
    
    // Handle click event
    handleClick(event) {
        const rect = this.canvas.getBoundingClientRect();
        const x = event.clientX - rect.left;
        const y = event.clientY - rect.top;
        
        if (this.mode === EditorMode.CREATE_INTERSECTION) {
            // Find roads near the click point
            const road1 = this.roadNetwork.findNearestRoadSegment(x, y);
            if (road1) {
                // Create a new intersection on this road
                const roadObj = this.roadNetwork.roadSegments.get(road1.id);
                
                // Determine if click is closer to start or end of road
                const distToStart = new Vector2D(x, y).distanceTo(roadObj.startPoint);
                const distToEnd = new Vector2D(x, y).distanceTo(roadObj.endPoint);
                const isEnd = distToStart > distToEnd;
                
                // Find another nearby road to connect with
                const excludeRoad = road1.id;
                let road2 = null;
                let minDist = this.snapDistance;
                
                for (const [id, road] of this.roadNetwork.roadSegments) {
                    if (id === excludeRoad) continue;
                    
                    const dist = Math.min(
                        new Vector2D(x, y).distanceTo(road.startPoint),
                        new Vector2D(x, y).distanceTo(road.endPoint)
                    );
                    
                    if (dist < minDist) {
                        minDist = dist;
                        road2 = { id, road };
                    }
                }
                
                if (road2) {
                    // Determine if click is closer to start or end of second road
                    const distToStart2 = new Vector2D(x, y).distanceTo(road2.road.startPoint);
                    const distToEnd2 = new Vector2D(x, y).distanceTo(road2.road.endPoint);
                    const isEnd2 = distToStart2 > distToEnd2;
                    
                    // Connect the two roads with an intersection
                    this.roadNetwork.connectWithIntersection(road1.id, isEnd, road2.id, isEnd2);
                    
                    if (this.onNetworkUpdate) this.onNetworkUpdate();
                } else {
                    // Just create an intersection on the one road
                    const intersectionId = this.roadNetwork.createIntersection(x, y);
                    const intersection = this.roadNetwork.intersections.get(intersectionId);
                    
                    // Connect the road to the intersection
                    intersection.connectRoad(roadObj, !isEnd);
                    
                    if (this.onNetworkUpdate) this.onNetworkUpdate();
                }
            }
        }
        
        this.render();
    }
    
    // Snap to nearby intersection or road endpoint
    snapToNearbyElement(x, y) {
        const point = new Vector2D(x, y);
        let minDist = this.snapDistance;
        let snapTarget = null;
        
        // Check intersections
        for (const [id, intersection] of this.roadNetwork.intersections) {
            const dist = point.distanceTo(intersection.position);
            if (dist < minDist) {
                minDist = dist;
                snapTarget = intersection.position;
            }
        }
        
        // Check road endpoints
        for (const [id, road] of this.roadNetwork.roadSegments) {
            const distToStart = point.distanceTo(road.startPoint);
            if (distToStart < minDist) {
                minDist = distToStart;
                snapTarget = road.startPoint;
            }
            
            const distToEnd = point.distanceTo(road.endPoint);
            if (distToEnd < minDist) {
                minDist = distToEnd;
                snapTarget = road.endPoint;
            }
        }
        
        return snapTarget;
    }
    
    // Render the editor
    render() {
        // Clear canvas
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
        
        // Draw grid if enabled
        if (this.showGrid) {
            this.renderGrid();
        }
        
        // Draw the road network
        this.roadNetwork.render(this.ctx);
        
        // Draw selection
        if (this.selectedElement) {
            this.renderSelection();
        }
        
        // Draw drag line if creating a road
        if (this.isDragging && this.mode === EditorMode.CREATE_ROAD && this.dragStart && this.dragEnd) {
            this.ctx.beginPath();
            this.ctx.moveTo(this.dragStart.x, this.dragStart.y);
            this.ctx.lineTo(this.dragEnd.x, this.dragEnd.y);
            this.ctx.strokeStyle = '#2196F3';
            this.ctx.lineWidth = 2;
            this.ctx.stroke();
            
            // Draw endpoints
            this.ctx.beginPath();
            this.ctx.arc(this.dragStart.x, this.dragStart.y, 5, 0, Math.PI * 2);
            this.ctx.fillStyle = '#2196F3';
            this.ctx.fill();
            
            this.ctx.beginPath();
            this.ctx.arc(this.dragEnd.x, this.dragEnd.y, 5, 0, Math.PI * 2);
            this.ctx.fillStyle = '#2196F3';
            this.ctx.fill();
        }
    }
    
    // Render the grid
    renderGrid() {
        const width = this.canvas.width;
        const height = this.canvas.height;
        
        this.ctx.strokeStyle = '#ddd';
        this.ctx.lineWidth = 0.5;
        
        // Draw vertical lines
        for (let x = 0; x < width; x += this.gridSize) {
            this.ctx.beginPath();
            this.ctx.moveTo(x, 0);
            this.ctx.lineTo(x, height);
            this.ctx.stroke();
        }
        
        // Draw horizontal lines
        for (let y = 0; y < height; y += this.gridSize) {
            this.ctx.beginPath();
            this.ctx.moveTo(0, y);
            this.ctx.lineTo(width, y);
            this.ctx.stroke();
        }
    }
    
    // Render selection highlight
    renderSelection() {
        if (!this.selectedElement) return;
        
        if (this.selectedElement.type === 'road') {
            const road = this.roadNetwork.roadSegments.get(this.selectedElement.id);
            if (!road) return;
            
            // Highlight the road
            this.ctx.beginPath();
            this.ctx.moveTo(road.startPoint.x, road.startPoint.y);
            this.ctx.lineTo(road.endPoint.x, road.endPoint.y);
            this.ctx.lineWidth = road.lanes.length * 8 + 4;
            this.ctx.strokeStyle = 'rgba(33, 150, 243, 0.3)';
            this.ctx.stroke();
            
            // Highlight endpoints
            this.ctx.beginPath();
            this.ctx.arc(road.startPoint.x, road.startPoint.y, 7, 0, Math.PI * 2);
            this.ctx.fillStyle = 'rgba(33, 150, 243, 0.5)';
            this.ctx.fill();
            
            this.ctx.beginPath();
            this.ctx.arc(road.endPoint.x, road.endPoint.y, 7, 0, Math.PI * 2);
            this.ctx.fillStyle = 'rgba(33, 150, 243, 0.5)';
            this.ctx.fill();
        } else if (this.selectedElement.type === 'intersection') {
            const intersection = this.roadNetwork.intersections.get(this.selectedElement.id);
            if (!intersection) return;
            
            // Highlight the intersection
            this.ctx.beginPath();
            this.ctx.arc(intersection.position.x, intersection.position.y, 14, 0, Math.PI * 2);
            this.ctx.fillStyle = 'rgba(33, 150, 243, 0.3)';
            this.ctx.fill();
            this.ctx.lineWidth = 3;
            this.ctx.strokeStyle = 'rgba(33, 150, 243, 0.7)';
            this.ctx.stroke();
        }
    }
    
    // Get the road network
    getRoadNetwork() {
        return this.roadNetwork;
    }
    
    // Save the road network to JSON
    saveToJSON() {
        // Create serializable versions of the road segments and intersections
        // that don't have circular references
        const serializableRoadSegments = [];
        
        for (const [id, road] of this.roadNetwork.roadSegments) {
            serializableRoadSegments.push({
                id: road.id,
                startPoint: { x: road.startPoint.x, y: road.startPoint.y },
                endPoint: { x: road.endPoint.x, y: road.endPoint.y },
                startIntersectionId: road.startIntersection ? road.startIntersection.id : null,
                endIntersectionId: road.endIntersection ? road.endIntersection.id : null,
                lanes: road.lanes.map(lane => ({
                    id: lane.id,
                    width: lane.width,
                    type: lane.type
                }))
            });
        }
        
        const serializableIntersections = [];
        
        for (const [id, intersection] of this.roadNetwork.intersections) {
            const connectedRoadIds = [];
            for (const connectedRoad of intersection.connectedRoads) {
                connectedRoadIds.push({
                    roadId: connectedRoad.road.id,
                    isStart: connectedRoad.isStart
                });
            }
            
            serializableIntersections.push({
                id: intersection.id,
                position: { x: intersection.position.x, y: intersection.position.y },
                connectedRoadIds: connectedRoadIds
            });
        }
        
        const data = {
            roadSegments: serializableRoadSegments,
            intersections: serializableIntersections,
            nextRoadId: this.roadNetwork.nextRoadId,
            nextIntersectionId: this.roadNetwork.nextIntersectionId
        };
        
        return JSON.stringify(data);
    }
    
    // Load the road network from JSON
    loadFromJSON(json) {
        try {
            const data = JSON.parse(json);
            
            // Clear current network
            this.roadNetwork = new RoadNetwork();
            
            // First pass: Load road segments and intersections without connections
            // Load road segments
            for (const roadData of data.roadSegments) {
                // Create road segment with start and end points
                const startPoint = new Vector2D(roadData.startPoint.x, roadData.startPoint.y);
                const endPoint = new Vector2D(roadData.endPoint.x, roadData.endPoint.y);
                
                // Create the road segment in the network
                this.roadNetwork.roadSegments.set(roadData.id, new RoadSegment(
                    roadData.id,
                    startPoint,
                    endPoint
                ));
                
                // Add lanes
                const road = this.roadNetwork.roadSegments.get(roadData.id);
                for (const laneData of roadData.lanes) {
                    if (road.lanes.length <= laneData.id) {
                        road.addLane(laneData.width, laneData.type);
                    }
                }
            }
            
            // Load intersections
            for (const intersectionData of data.intersections) {
                const position = new Vector2D(intersectionData.position.x, intersectionData.position.y);
                this.roadNetwork.intersections.set(intersectionData.id, new Intersection(
                    intersectionData.id,
                    position
                ));
            }
            
            // Second pass: Connect roads and intersections
            for (const roadData of data.roadSegments) {
                const road = this.roadNetwork.roadSegments.get(roadData.id);
                
                // Connect to start intersection if any
                if (roadData.startIntersectionId !== null) {
                    const intersection = this.roadNetwork.intersections.get(roadData.startIntersectionId);
                    if (intersection) {
                        road.startIntersection = intersection;
                    }
                }
                
                // Connect to end intersection if any
                if (roadData.endIntersectionId !== null) {
                    const intersection = this.roadNetwork.intersections.get(roadData.endIntersectionId);
                    if (intersection) {
                        road.endIntersection = intersection;
                    }
                }
            }
            
            // Connect intersections to roads
            for (const intersectionData of data.intersections) {
                const intersection = this.roadNetwork.intersections.get(intersectionData.id);
                
                for (const connectedRoadData of intersectionData.connectedRoadIds) {
                    const road = this.roadNetwork.roadSegments.get(connectedRoadData.roadId);
                    if (road) {
                        intersection.connectRoad(road, connectedRoadData.isStart);
                    }
                }
            }
            
            // Set next IDs
            this.roadNetwork.nextRoadId = data.nextRoadId;
            this.roadNetwork.nextIntersectionId = data.nextIntersectionId;
            
            // Set next IDs
            this.roadNetwork.nextRoadId = data.nextRoadId;
            this.roadNetwork.nextIntersectionId = data.nextIntersectionId;
            
            // Render the loaded network
            this.render();
            
            // Notify of update
            if (this.onNetworkUpdate) this.onNetworkUpdate();
            
            return true;
        } catch (error) {
            console.error('Error loading road network:', error);
            return false;
        }
    }
    
    // Create a predefined network (for demo purposes)
    createDemoNetwork() {
        // Clear existing network
        this.roadNetwork = new RoadNetwork();
        
        // Create a simple grid network
        const size = 200;
        const offset = 100;
        
        // Create horizontal roads
        const road1 = this.roadNetwork.createRoadSegment(offset, offset, offset + size, offset);
        const road2 = this.roadNetwork.createRoadSegment(offset, offset + size, offset + size, offset + size);
        
        // Create vertical roads
        const road3 = this.roadNetwork.createRoadSegment(offset, offset, offset, offset + size);
        const road4 = this.roadNetwork.createRoadSegment(offset + size, offset, offset + size, offset + size);
        
        // Create intersections
        this.roadNetwork.connectWithIntersection(road1, false, road4, false); // Top-right
        this.roadNetwork.connectWithIntersection(road1, true, road3, true);   // Top-left
        this.roadNetwork.connectWithIntersection(road2, true, road3, false);  // Bottom-left
        this.roadNetwork.connectWithIntersection(road2, false, road4, true);  // Bottom-right
        
        // Add more complex roads
        const road5 = this.roadNetwork.createRoadSegment(offset + size, offset + size / 2, offset + size + size / 2, offset + size / 2);
        
        // Add a lane to some roads
        const road = this.roadNetwork.roadSegments.get(road1);
        road.addLane();
        
        // Render the network
        this.render();
        
        // Notify of update
        if (this.onNetworkUpdate) this.onNetworkUpdate();
    }
}

// Export the road editor
export { RoadEditor, EditorMode };