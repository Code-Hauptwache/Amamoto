#include "RoadNetwork.h"
#include <queue>
#include <limits>
#include <algorithm>
#include <cmath>

namespace amamoto {

// Lane implementation
Lane::Lane(LaneID id, RoadSegment* parent, float width, LaneType type)
    : m_id(id)
    , m_parent(parent)
    , m_width(width)
    , m_type(type)
    , m_boundariesDirty(true)
{
}

std::vector<Vector2D> Lane::getCenterLine() const {
    return m_centerLine;
}

std::vector<Vector2D> Lane::getLeftBoundary() const {
    if (m_boundariesDirty) {
        updateBoundaries();
    }
    return m_leftBoundary;
}

std::vector<Vector2D> Lane::getRightBoundary() const {
    if (m_boundariesDirty) {
        updateBoundaries();
    }
    return m_rightBoundary;
}

bool Lane::canBeUsedBy(const Entity& vehicle) const {
    // Check if vehicle type can use this lane
    // For now, assume all vehicles can use driving lanes
    return m_type == LaneType::DRIVING;
}

void Lane::updateBoundaries() const {
    m_leftBoundary.clear();
    m_rightBoundary.clear();
    
    if (m_centerLine.empty()) {
        return;
    }
    
    const float halfWidth = m_width / 2.0f;
    
    for (size_t i = 0; i < m_centerLine.size(); ++i) {
        Vector2D perpendicular;
        
        if (i == 0) {
            // Use the direction to the next point for the first point
            Vector2D direction = (m_centerLine[1] - m_centerLine[0]).normalized();
            perpendicular = Vector2D(-direction.y, direction.x);
        }
        else if (i == m_centerLine.size() - 1) {
            // Use the direction from the previous point for the last point
            Vector2D direction = (m_centerLine[i] - m_centerLine[i-1]).normalized();
            perpendicular = Vector2D(-direction.y, direction.x);
        }
        else {
            // Use the average direction for middle points
            Vector2D dir1 = (m_centerLine[i] - m_centerLine[i-1]).normalized();
            Vector2D dir2 = (m_centerLine[i+1] - m_centerLine[i]).normalized();
            Vector2D avgDir = (dir1 + dir2).normalized();
            perpendicular = Vector2D(-avgDir.y, avgDir.x);
        }
        
        m_leftBoundary.push_back(m_centerLine[i] + perpendicular * halfWidth);
        m_rightBoundary.push_back(m_centerLine[i] - perpendicular * halfWidth);
    }
    
    m_boundariesDirty = false;
}

// Road Segment implementation
RoadSegment::RoadSegment(RoadSegmentID id, const ConnectionPoint& start, const ConnectionPoint& end)
    : m_id(id)
    , m_startPoint(start)
    , m_endPoint(end)
    , m_startIntersection(nullptr)
    , m_endIntersection(nullptr)
{
    // Calculate length
    m_length = (end.position - start.position).length();
    
    // For simplicity, we'll use a straight line for the center line
    // In a more complex implementation, this could be a curve
    m_centerLine.push_back(start.position);
    m_centerLine.push_back(end.position);
}

LaneID RoadSegment::addLane(float width, LaneType type) {
    auto lane = std::make_unique<Lane>(m_lanes.size(), this, width, type);
    m_lanes.push_back(std::move(lane));
    return m_lanes.size() - 1;
}

Lane* RoadSegment::getLane(LaneID id) {
    if (id < m_lanes.size()) {
        return m_lanes[id].get();
    }
    return nullptr;
}

float RoadSegment::getLength() const {
    return m_length;
}

Vector2D RoadSegment::getPointAtDistance(float distance) const {
    // Clamp distance to [0, length]
    distance = std::max(0.0f, std::min(distance, m_length));
    
    // For a straight line, this is simple interpolation
    float t = distance / m_length;
    return m_startPoint.position + (m_endPoint.position - m_startPoint.position) * t;
}

Vector2D RoadSegment::getDirectionAtDistance(float distance) const {
    // For straight lines, direction is constant
    return (m_endPoint.position - m_startPoint.position).normalized();
}

// Traffic Signal implementation
TrafficSignal::TrafficSignal()
    : m_greenTime(30.0f)
    , m_yellowTime(5.0f)
    , m_redTime(30.0f)
    , m_cycleTime(65.0f)
    , m_timer(0.0f)
    , m_currentState(SignalState::GREEN)
{
}

void TrafficSignal::configure(float greenTime, float yellowTime, float redTime) {
    m_greenTime = greenTime;
    m_yellowTime = yellowTime;
    m_redTime = redTime;
    m_cycleTime = greenTime + yellowTime + redTime;
}

void TrafficSignal::update(float dt) {
    m_timer += dt;
    
    // Wrap around if we've completed a cycle
    while (m_timer >= m_cycleTime) {
        m_timer -= m_cycleTime;
    }
    
    // Update state based on timer
    if (m_timer < m_greenTime) {
        m_currentState = SignalState::GREEN;
    }
    else if (m_timer < m_greenTime + m_yellowTime) {
        m_currentState = SignalState::YELLOW;
    }
    else {
        m_currentState = SignalState::RED;
    }
}

float TrafficSignal::getTimeUntilChange() const {
    if (m_currentState == SignalState::GREEN) {
        return m_greenTime - m_timer;
    }
    else if (m_currentState == SignalState::YELLOW) {
        return (m_greenTime + m_yellowTime) - m_timer;
    }
    else {
        return m_cycleTime - m_timer;
    }
}

// Intersection implementation
Intersection::Intersection(IntersectionID id, const Vector2D& position)
    : m_id(id)
    , m_position(position)
{
}

void Intersection::connectRoadSegment(RoadSegment* roadSegment, bool isStart) {
    m_connectedRoadSegments.push_back(roadSegment);
    
    // Connect the road segment to this intersection
    if (isStart) {
        roadSegment->setStartIntersection(this);
    }
    else {
        roadSegment->setEndIntersection(this);
    }
}

void Intersection::disconnectRoadSegment(RoadSegmentID id) {
    // Find and remove the road segment
    m_connectedRoadSegments.erase(
        std::remove_if(m_connectedRoadSegments.begin(), m_connectedRoadSegments.end(),
            [id](const RoadSegment* road) { return road->getID() == id; }),
        m_connectedRoadSegments.end()
    );
    
    // Remove connections involving this road
    m_connections.erase(id);
    
    // Remove connections to this road from other roads
    for (auto& [roadID, connections] : m_connections) {
        for (auto& connection : connections) {
            connection.allowedDestinations.erase(
                std::remove_if(connection.allowedDestinations.begin(), connection.allowedDestinations.end(),
                    [id](const std::pair<RoadSegmentID, LaneID>& dest) { return dest.first == id; }),
                connection.allowedDestinations.end()
            );
        }
    }
}

void Intersection::defineConnection(RoadSegmentID inRoadID, LaneID inLaneID, 
                                  RoadSegmentID outRoadID, LaneID outLaneID) {
    // Find or create the connection
    auto& connections = m_connections[inRoadID];
    
    // Find the connection for this lane
    auto it = std::find_if(connections.begin(), connections.end(),
        [inLaneID](const IntersectionConnection& conn) { return conn.laneID == inLaneID; });
    
    if (it == connections.end()) {
        // Create a new connection
        IntersectionConnection newConn;
        newConn.roadSegmentID = inRoadID;
        newConn.laneID = inLaneID;
        connections.push_back(newConn);
        it = connections.end() - 1;
    }
    
    // Add the destination
    it->allowedDestinations.push_back(std::make_pair(outRoadID, outLaneID));
}

void Intersection::updateSignals(float dt) {
    for (auto& [roadID, connections] : m_connections) {
        for (auto& connection : connections) {
            connection.signal.update(dt);
        }
    }
}

void Intersection::configureSignalTiming(float greenTime, float yellowTime, float redTime) {
    for (auto& [roadID, connections] : m_connections) {
        for (auto& connection : connections) {
            connection.signal.configure(greenTime, yellowTime, redTime);
        }
    }
}

// Road Network implementation
RoadNetwork::RoadNetwork()
    : m_nextRoadSegmentID(0)
    , m_nextIntersectionID(0)
    , m_nextLaneID(0)
{
}

RoadNetwork::~RoadNetwork() {
    // Clear all connections to avoid dangling pointers
    for (auto& [id, intersection] : m_intersections) {
        intersection->m_connectedRoadSegments.clear();
    }
    
    for (auto& [id, roadSegment] : m_roadSegments) {
        roadSegment->setStartIntersection(nullptr);
        roadSegment->setEndIntersection(nullptr);
    }
}

RoadSegmentID RoadNetwork::createRoadSegment(const ConnectionPoint& start, const ConnectionPoint& end) {
    RoadSegmentID id = m_nextRoadSegmentID++;
    m_roadSegments[id] = std::make_unique<RoadSegment>(id, start, end);
    
    // Add default lane
    m_roadSegments[id]->addLane(3.5f, LaneType::DRIVING);
    
    return id;
}

IntersectionID RoadNetwork::createIntersection(const Vector2D& position) {
    IntersectionID id = m_nextIntersectionID++;
    m_intersections[id] = std::make_unique<Intersection>(id, position);
    return id;
}

void RoadNetwork::connectWithIntersection(RoadSegmentID road1, bool road1End, 
                                       RoadSegmentID road2, bool road2End) {
    // Get the road segments
    auto road1Ptr = getRoadSegment(road1);
    auto road2Ptr = getRoadSegment(road2);
    
    if (!road1Ptr || !road2Ptr) {
        return;
    }
    
    // Get the connection points
    const ConnectionPoint& p1 = road1End ? road1Ptr->getEndPoint() : road1Ptr->getStartPoint();
    const ConnectionPoint& p2 = road2End ? road2Ptr->getEndPoint() : road2Ptr->getStartPoint();
    
    // Create an intersection at the midpoint
    Vector2D midpoint = (p1.position + p2.position) * 0.5f;
    IntersectionID intersectionID = createIntersection(midpoint);
    
    auto intersection = getIntersection(intersectionID);
    if (!intersection) {
        return;
    }
    
    // Connect the roads to the intersection
    intersection->connectRoadSegment(road1Ptr, !road1End);
    intersection->connectRoadSegment(road2Ptr, !road2End);
    
    // Define default connections for all lanes
    for (size_t lane1 = 0; lane1 < road1Ptr->getLanes().size(); ++lane1) {
        for (size_t lane2 = 0; lane2 < road2Ptr->getLanes().size(); ++lane2) {
            intersection->defineConnection(road1, lane1, road2, lane2);
            intersection->defineConnection(road2, lane2, road1, lane1);
        }
    }
}

RoadSegment* RoadNetwork::getRoadSegment(RoadSegmentID id) const {
    auto it = m_roadSegments.find(id);
    if (it != m_roadSegments.end()) {
        return it->second.get();
    }
    return nullptr;
}

Intersection* RoadNetwork::getIntersection(IntersectionID id) const {
    auto it = m_intersections.find(id);
    if (it != m_intersections.end()) {
        return it->second.get();
    }
    return nullptr;
}

// Simple A* pathfinding implementation
std::vector<std::pair<RoadSegmentID, LaneID>> RoadNetwork::findPath(
    const Vector2D& start, const Vector2D& end) {
    
    std::vector<std::pair<RoadSegmentID, LaneID>> path;
    
    // Find nearest road segments to start and end points
    RoadSegmentID startRoadID = 0;
    RoadSegmentID endRoadID = 0;
    float minStartDist = std::numeric_limits<float>::max();
    float minEndDist = std::numeric_limits<float>::max();
    
    for (const auto& [id, road] : m_roadSegments) {
        // Find distance to start point
        float startDist = std::min(
            (road->getStartPoint().position - start).length(),
            (road->getEndPoint().position - start).length()
        );
        
        if (startDist < minStartDist) {
            minStartDist = startDist;
            startRoadID = id;
        }
        
        // Find distance to end point
        float endDist = std::min(
            (road->getStartPoint().position - end).length(),
            (road->getEndPoint().position - end).length()
        );
        
        if (endDist < minEndDist) {
            minEndDist = endDist;
            endRoadID = id;
        }
    }
    
    if (startRoadID == endRoadID) {
        // Start and end are on the same road
        path.push_back(std::make_pair(startRoadID, 0)); // Assume first lane
        return path;
    }
    
    // A* search
    struct Node {
        RoadSegmentID roadID;
        IntersectionID intersectionID;
        float gScore;
        float fScore;
        
        // For the priority queue
        bool operator>(const Node& other) const {
            return fScore > other.fScore;
        }
    };
    
    struct NodePath {
        std::vector<std::pair<RoadSegmentID, LaneID>> path;
        IntersectionID lastIntersection;
    };
    
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
    std::unordered_map<RoadSegmentID, float> gScore;
    std::unordered_map<RoadSegmentID, NodePath> cameFrom;
    
    // Initialize start node
    openSet.push({ startRoadID, 0, 0.0f, (m_roadSegments[startRoadID]->getEndPoint().position - end).length() });
    gScore[startRoadID] = 0.0f;
    
    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();
        
        if (current.roadID == endRoadID) {
            // We found the target, reconstruct path
            auto pathIt = cameFrom.find(current.roadID);
            if (pathIt != cameFrom.end()) {
                return pathIt->second.path;
            }
            break;
        }
        
        // Get the road segment
        RoadSegment* road = getRoadSegment(current.roadID);
        if (!road) continue;
        
        // Check both endpoints
        Intersection* startIntersection = road->getStartIntersection();
        Intersection* endIntersection = road->getEndIntersection();
        
        // Skip the intersection we came from
        if (startIntersection && startIntersection->getID() != current.intersectionID) {
            // Process all connected roads at this intersection
            for (RoadSegment* nextRoad : startIntersection->getConnectedRoadSegments()) {
                if (nextRoad->getID() == current.roadID) continue; // Skip current road
                
                float tentativeGScore = gScore[current.roadID] + road->getLength();
                
                if (!gScore.count(nextRoad->getID()) || tentativeGScore < gScore[nextRoad->getID()]) {
                    // This is a better path
                    gScore[nextRoad->getID()] = tentativeGScore;
                    float hScore = (nextRoad->getEndPoint().position - end).length();
                    
                    // Update path
                    NodePath newPath;
                    if (cameFrom.count(current.roadID)) {
                        newPath.path = cameFrom[current.roadID].path;
                    }
                    newPath.path.push_back(std::make_pair(current.roadID, 0)); // Assume first lane
                    newPath.lastIntersection = startIntersection->getID();
                    cameFrom[nextRoad->getID()] = newPath;
                    
                    // Add to open set
                    openSet.push({ nextRoad->getID(), startIntersection->getID(), tentativeGScore, tentativeGScore + hScore });
                }
            }
        }
        
        if (endIntersection && endIntersection->getID() != current.intersectionID) {
            // Process all connected roads at this intersection
            for (RoadSegment* nextRoad : endIntersection->getConnectedRoadSegments()) {
                if (nextRoad->getID() == current.roadID) continue; // Skip current road
                
                float tentativeGScore = gScore[current.roadID] + road->getLength();
                
                if (!gScore.count(nextRoad->getID()) || tentativeGScore < gScore[nextRoad->getID()]) {
                    // This is a better path
                    gScore[nextRoad->getID()] = tentativeGScore;
                    float hScore = (nextRoad->getEndPoint().position - end).length();
                    
                    // Update path
                    NodePath newPath;
                    if (cameFrom.count(current.roadID)) {
                        newPath.path = cameFrom[current.roadID].path;
                    }
                    newPath.path.push_back(std::make_pair(current.roadID, 0)); // Assume first lane
                    newPath.lastIntersection = endIntersection->getID();
                    cameFrom[nextRoad->getID()] = newPath;
                    
                    // Add to open set
                    openSet.push({ nextRoad->getID(), endIntersection->getID(), tentativeGScore, tentativeGScore + hScore });
                }
            }
        }
    }
    
    // No path found
    return path;
}

void RoadNetwork::update(float dt) {
    // Update all intersections
    for (auto& [id, intersection] : m_intersections) {
        intersection->updateSignals(dt);
    }
}

// PathFollowingComponent implementation
void PathFollowingComponent::setPath(const std::vector<std::pair<RoadSegmentID, LaneID>>& path) {
    m_path = path;
    m_currentPathIndex = 0;
    m_distanceAlongCurrentSegment = 0.0f;
}

void PathFollowingComponent::update(float dt, TransformComponent& transform, VehicleComponent& vehicle, const RoadNetwork& roadNetwork) {
    if (hasReachedDestination()) {
        // Stop the vehicle
        vehicle.targetSpeed = 0.0f;
        return;
    }
    
    // Get current road segment and lane
    auto [roadID, laneID] = m_path[m_currentPathIndex];
    RoadSegment* road = roadNetwork.getRoadSegment(roadID);
    
    if (!road) {
        // Invalid road, skip to next
        m_currentPathIndex++;
        return;
    }
    
    // Calculate target position
    float lookAheadDistance = vehicle.currentSpeed * 2.0f + 5.0f; // Look ahead more at higher speeds
    float targetDistance = m_distanceAlongCurrentSegment + lookAheadDistance;
    
    // Check if we need to move to the next road segment
    if (targetDistance > road->getLength()) {
        // We've reached the end of this segment
        m_currentPathIndex++;
        
        if (hasReachedDestination()) {
            // Stop the vehicle
            vehicle.targetSpeed = 0.0f;
            return;
        }
        
        // Reset distance for the next segment
        m_distanceAlongCurrentSegment = 0.0f;
        targetDistance = lookAheadDistance;
        
        // Get the new road segment
        auto [nextRoadID, nextLaneID] = m_path[m_currentPathIndex];
        road = roadNetwork.getRoadSegment(nextRoadID);
        
        if (!road) {
            // Invalid road
            return;
        }
    }
    
    // Get target position and direction
    Vector2D targetPosition = road->getPointAtDistance(targetDistance);
    Vector2D targetDirection = road->getDirectionAtDistance(targetDistance);
    
    // Calculate steering force
    Vector2D steeringForce = calculateSteeringForce(
        transform.position,
        transform.velocity, 
        targetPosition,
        vehicle.maxSpeed
    );
    
    // Apply steering
    transform.velocity = transform.velocity + steeringForce * dt;
    
    // Limit velocity to max speed
    float speed = transform.velocity.length();
    if (speed > vehicle.maxSpeed) {
        transform.velocity = transform.velocity.normalized() * vehicle.maxSpeed;
    }
    
    // Update distance along segment
    m_distanceAlongCurrentSegment += vehicle.currentSpeed * dt;
    
    // Set target speed based on conditions (traffic signals, curves, etc.)
    vehicle.targetSpeed = vehicle.maxSpeed;
}

Vector2D PathFollowingComponent::calculateSteeringForce(
    const Vector2D& currentPos, 
    const Vector2D& currentVel, 
    const Vector2D& targetPos, 
    float maxSpeed) {
    
    // Desired velocity is towards the target at max speed
    Vector2D desiredVelocity = (targetPos - currentPos).normalized() * maxSpeed;
    
    // Steering force is the difference between desired and current velocity
    return desiredVelocity - currentVel;
}

} // namespace amamoto