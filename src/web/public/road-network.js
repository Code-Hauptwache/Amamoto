// Road Network implementation for the traffic simulation
// This is the JavaScript version of our C++ road network

// Vector2D utility class
class Vector2D {
    constructor(x = 0, y = 0) {
        this.x = x;
        this.y = y;
    }

    add(other) {
        return new Vector2D(this.x + other.x, this.y + other.y);
    }

    subtract(other) {
        return new Vector2D(this.x - other.x, this.y - other.y);
    }

    multiply(scalar) {
        return new Vector2D(this.x * scalar, this.y * scalar);
    }

    length() {
        return Math.sqrt(this.x * this.x + this.y * this.y);
    }

    normalize() {
        const len = this.length();
        if (len < 0.0001) return new Vector2D(0, 0);
        return new Vector2D(this.x / len, this.y / len);
    }

    distanceTo(other) {
        const dx = this.x - other.x;
        const dy = this.y - other.y;
        return Math.sqrt(dx * dx + dy * dy);
    }
}

// Road segment connecting two points
class RoadSegment {
    constructor(id, startPoint, endPoint) {
        this.id = id;
        this.startPoint = startPoint;
        this.endPoint = endPoint;
        this.startIntersection = null;
        this.endIntersection = null;
        this.lanes = [];
        this.length = startPoint.distanceTo(endPoint);
    }

    // Add a lane to the road segment
    addLane(width = 3.5, type = 'driving') {
        const laneId = this.lanes.length;
        const lane = {
            id: laneId,
            width: width,
            type: type
        };
        this.lanes.push(lane);
        return laneId;
    }

    // Get a point along the road segment at a given distance
    getPointAtDistance(distance) {
        // Clamp distance to road length
        distance = Math.max(0, Math.min(distance, this.length));
        
        // Linear interpolation between start and end points
        const t = distance / this.length;
        const x = this.startPoint.x + (this.endPoint.x - this.startPoint.x) * t;
        const y = this.startPoint.y + (this.endPoint.y - this.startPoint.y) * t;
        
        return new Vector2D(x, y);
    }

    // Get the direction at a given distance
    getDirectionAtDistance(distance) {
        // For a straight road, direction is constant
        const direction = new Vector2D(
            this.endPoint.x - this.startPoint.x,
            this.endPoint.y - this.startPoint.y
        );
        return direction.normalize();
    }

    // Get a lane position (offset from road center) at a given distance
    getLanePositionAtDistance(laneId, distance) {
        if (laneId >= this.lanes.length) return null;
        
        const point = this.getPointAtDistance(distance);
        const direction = this.getDirectionAtDistance(distance);
        
        // Calculate perpendicular direction (normal)
        const normal = new Vector2D(-direction.y, direction.x);
        
        // Calculate lane offset from center
        let offset = 0;
        const laneWidth = this.lanes[laneId].width;
        
        // Calculate offset based on lane index
        // Lanes with even indices go to the right, odd indices to the left
        if (laneId % 2 === 0) {
            offset = (laneId / 2) * laneWidth + laneWidth / 2;
        } else {
            offset = -((laneId + 1) / 2) * laneWidth - laneWidth / 2;
        }
        
        // Apply offset
        return point.add(normal.multiply(offset));
    }
}

// Intersection connecting multiple road segments
class Intersection {
    constructor(id, position) {
        this.id = id;
        this.position = position;
        this.connectedRoads = [];
        this.connections = new Map(); // Maps from (incomingRoadId, laneId) to array of (outgoingRoadId, laneId)
        this.trafficSignals = new Map(); // Maps from (roadId, laneId) to signal state
    }

    // Connect a road segment to this intersection
    connectRoad(roadSegment, isStart) {
        this.connectedRoads.push({
            road: roadSegment,
            isStart: isStart
        });
        
        // Update the road's reference to this intersection
        if (isStart) {
            roadSegment.startIntersection = this;
        } else {
            roadSegment.endIntersection = this;
        }
    }

    // Define a connection from an incoming lane to outgoing lanes
    defineConnection(inRoadId, inLaneId, outRoadId, outLaneId) {
        const key = `${inRoadId}_${inLaneId}`;
        
        if (!this.connections.has(key)) {
            this.connections.set(key, []);
        }
        
        this.connections.get(key).push({
            roadId: outRoadId,
            laneId: outLaneId
        });
    }

    // Update traffic signals
    updateSignals(dt) {
        // For this simple implementation, we'll just toggle signals every 30 seconds
        // A more sophisticated version would coordinate signals across lanes
    }
}

// Main road network class
class RoadNetwork {
    constructor() {
        this.roadSegments = new Map();
        this.intersections = new Map();
        this.nextRoadId = 0;
        this.nextIntersectionId = 0;
    }

    // Create a new road segment
    createRoadSegment(startX, startY, endX, endY) {
        const id = this.nextRoadId++;
        const startPoint = new Vector2D(startX, startY);
        const endPoint = new Vector2D(endX, endY);
        
        const roadSegment = new RoadSegment(id, startPoint, endPoint);
        
        // Add default lane
        roadSegment.addLane();
        
        this.roadSegments.set(id, roadSegment);
        return id;
    }

    // Create a new intersection
    createIntersection(x, y) {
        const id = this.nextIntersectionId++;
        const position = new Vector2D(x, y);
        
        const intersection = new Intersection(id, position);
        this.intersections.set(id, intersection);
        
        return id;
    }

    // Connect two road segments with an intersection
    connectWithIntersection(road1Id, road1End, road2Id, road2End) {
        const road1 = this.roadSegments.get(road1Id);
        const road2 = this.roadSegments.get(road2Id);
        
        if (!road1 || !road2) return null;
        
        // Get the connection points
        const p1 = road1End ? road1.endPoint : road1.startPoint;
        const p2 = road2End ? road2.endPoint : road2.startPoint;
        
        // Create an intersection at the midpoint
        const midpoint = new Vector2D(
            (p1.x + p2.x) / 2,
            (p1.y + p2.y) / 2
        );
        
        const intersectionId = this.createIntersection(midpoint.x, midpoint.y);
        const intersection = this.intersections.get(intersectionId);
        
        // Connect roads to the intersection
        intersection.connectRoad(road1, !road1End);
        intersection.connectRoad(road2, !road2End);
        
        // Define default connections between all lanes
        for (let lane1 = 0; lane1 < road1.lanes.length; lane1++) {
            for (let lane2 = 0; lane2 < road2.lanes.length; lane2++) {
                intersection.defineConnection(road1.id, lane1, road2.id, lane2);
                intersection.defineConnection(road2.id, lane2, road1.id, lane1);
            }
        }
        
        return intersectionId;
    }

    // Find the nearest road segment to a point
    findNearestRoadSegment(x, y, maxDistance = 50) {
        const point = new Vector2D(x, y);
        let nearestRoad = null;
        let nearestDistance = maxDistance;
        
        for (const [id, road] of this.roadSegments) {
            // Calculate distance from point to line segment
            const roadVec = road.endPoint.subtract(road.startPoint);
            const roadLength = roadVec.length();
            const roadDir = roadVec.multiply(1 / roadLength);
            
            const pointVec = point.subtract(road.startPoint);
            const projection = pointVec.x * roadDir.x + pointVec.y * roadDir.y;
            
            let distance;
            if (projection < 0) {
                // Point is before start of road
                distance = point.distanceTo(road.startPoint);
            } else if (projection > roadLength) {
                // Point is after end of road
                distance = point.distanceTo(road.endPoint);
            } else {
                // Point is alongside road
                const projectionPoint = road.startPoint.add(roadDir.multiply(projection));
                distance = point.distanceTo(projectionPoint);
            }
            
            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearestRoad = { id, road, distance };
            }
        }
        
        return nearestRoad;
    }

    // Find the nearest intersection to a point
    findNearestIntersection(x, y, maxDistance = 50) {
        const point = new Vector2D(x, y);
        let nearestIntersection = null;
        let nearestDistance = maxDistance;
        
        for (const [id, intersection] of this.intersections) {
            const distance = point.distanceTo(intersection.position);
            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearestIntersection = { id, intersection, distance };
            }
        }
        
        return nearestIntersection;
    }

    // Find path between two points
    findPath(startX, startY, endX, endY) {
        // Find nearest road segments to start and end points
        const startRoad = this.findNearestRoadSegment(startX, startY);
        const endRoad = this.findNearestRoadSegment(endX, endY);
        
        if (!startRoad || !endRoad) return [];
        
        // If start and end are on the same road
        if (startRoad.id === endRoad.id) {
            return [{ roadId: startRoad.id, laneId: 0 }];
        }
        
        // A* search
        const openSet = new PriorityQueue();
        const gScore = new Map();
        const cameFrom = new Map();
        
        gScore.set(startRoad.id, 0);
        openSet.enqueue(startRoad.id, this.heuristic(startRoad.road, endRoad.road));
        
        while (!openSet.isEmpty()) {
            const current = openSet.dequeue();
            
            if (current === endRoad.id) {
                // Found path, reconstruct it
                return this.reconstructPath(cameFrom, current);
            }
            
            const currentRoad = this.roadSegments.get(current);
            
            // Process both ends of the road
            for (const isEnd of [true, false]) {
                const intersection = isEnd ? currentRoad.endIntersection : currentRoad.startIntersection;
                if (!intersection) continue;
                
                // Try all connected roads
                for (const conn of intersection.connectedRoads) {
                    if (conn.road.id === current) continue; // Skip current road
                    
                    const neighbor = conn.road.id;
                    const tentativeGScore = gScore.get(current) + currentRoad.length;
                    
                    if (!gScore.has(neighbor) || tentativeGScore < gScore.get(neighbor)) {
                        // This is a better path
                        cameFrom.set(neighbor, current);
                        gScore.set(neighbor, tentativeGScore);
                        const fScore = tentativeGScore + this.heuristic(conn.road, endRoad.road);
                        
                        if (!openSet.includes(neighbor)) {
                            openSet.enqueue(neighbor, fScore);
                        }
                    }
                }
            }
        }
        
        // No path found
        return [];
    }

    // Heuristic for A* (straight-line distance)
    heuristic(roadA, roadB) {
        const midA = new Vector2D(
            (roadA.startPoint.x + roadA.endPoint.x) / 2,
            (roadA.startPoint.y + roadA.endPoint.y) / 2
        );
        
        const midB = new Vector2D(
            (roadB.startPoint.x + roadB.endPoint.x) / 2,
            (roadB.startPoint.y + roadB.endPoint.y) / 2
        );
        
        return midA.distanceTo(midB);
    }

    // Reconstruct path from cameFrom map
    reconstructPath(cameFrom, current) {
        const path = [{ roadId: current, laneId: 0 }];
        
        while (cameFrom.has(current)) {
            current = cameFrom.get(current);
            path.unshift({ roadId: current, laneId: 0 });
        }
        
        return path;
    }

    // Update the road network
    update(dt) {
        // Update all intersections (traffic signals, etc.)
        for (const [id, intersection] of this.intersections) {
            intersection.updateSignals(dt);
        }
    }

    // Render the road network onto a canvas context
    render(ctx) {
        // Render all road segments
        for (const [id, road] of this.roadSegments) {
            // Draw road
            ctx.beginPath();
            ctx.moveTo(road.startPoint.x, road.startPoint.y);
            ctx.lineTo(road.endPoint.x, road.endPoint.y);
            ctx.lineWidth = road.lanes.length * 8; // Width based on lane count
            ctx.strokeStyle = '#444';
            ctx.stroke();
            
            // Draw center line
            ctx.beginPath();
            ctx.moveTo(road.startPoint.x, road.startPoint.y);
            ctx.lineTo(road.endPoint.x, road.endPoint.y);
            ctx.lineWidth = 1;
            ctx.strokeStyle = '#FFF';
            ctx.setLineDash([5, 5]);
            ctx.stroke();
            ctx.setLineDash([]);
        }
        
        // Render all intersections
        for (const [id, intersection] of this.intersections) {
            ctx.beginPath();
            ctx.arc(intersection.position.x, intersection.position.y, 10, 0, Math.PI * 2);
            ctx.fillStyle = '#555';
            ctx.fill();
            ctx.lineWidth = 2;
            ctx.strokeStyle = '#FFF';
            ctx.stroke();
        }
    }
}

// Simple priority queue for A* pathfinding
class PriorityQueue {
    constructor() {
        this.elements = [];
    }
    
    enqueue(element, priority) {
        this.elements.push({ element, priority });
        this.elements.sort((a, b) => a.priority - b.priority);
    }
    
    dequeue() {
        return this.elements.shift().element;
    }
    
    isEmpty() {
        return this.elements.length === 0;
    }
    
    includes(element) {
        return this.elements.some(item => item.element === element);
    }
}

export { Vector2D, RoadSegment, Intersection, RoadNetwork };