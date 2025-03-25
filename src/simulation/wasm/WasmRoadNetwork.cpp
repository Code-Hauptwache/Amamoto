#include "WasmRoadNetwork.h"
#include <cmath>
#include <limits>
#include <queue>
#include <algorithm>

namespace amamoto {

// WasmLane implementation
WasmLane::WasmLane(WasmLaneID id, WasmRoadSegment* parent, float width, WasmLaneType type)
    : m_id(id)
    , m_parent(parent)
    , m_width(width)
    , m_type(type)
{
}

// WasmRoadSegment implementation
WasmRoadSegment::WasmRoadSegment(WasmRoadSegmentID id, const WasmVector2D& startPoint, const WasmVector2D& endPoint)
    : m_id(id)
    , m_startPoint(startPoint)
    , m_endPoint(endPoint)
    , m_startIntersection(nullptr)
    , m_endIntersection(nullptr)
{
    // Calculate length
    float dx = endPoint.x - startPoint.x;
    float dy = endPoint.y - startPoint.y;
    m_length = std::sqrt(dx * dx + dy * dy);
}

float WasmRoadSegment::getWidth() const {
    float totalWidth = 0.0f;
    for (const auto& lane : m_lanes) {
        totalWidth += lane.getWidth();
    }
    return totalWidth;
}

WasmLaneID WasmRoadSegment::addLane(float width, WasmLaneType type) {
    WasmLaneID laneId = static_cast<WasmLaneID>(m_lanes.size());
    m_lanes.emplace_back(laneId, this, width, type);
    return laneId;
}

WasmVector2D WasmRoadSegment::getPointAtDistance(float distance) const {
    // Clamp distance to road length
    distance = std::max(0.0f, std::min(distance, m_length));
    
    // Linear interpolation between start and end points
    float t = distance / m_length;
    float x = m_startPoint.x + (m_endPoint.x - m_startPoint.x) * t;
    float y = m_startPoint.y + (m_endPoint.y - m_startPoint.y) * t;
    
    return WasmVector2D(x, y);
}

WasmVector2D WasmRoadSegment::getDirectionAtDistance(float distance) const {
    // For a straight road, direction is constant
    WasmVector2D direction(
        m_endPoint.x - m_startPoint.x,
        m_endPoint.y - m_startPoint.y
    );
    
    // Normalize
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length > 0.0001f) {
        direction.x /= length;
        direction.y /= length;
    }
    
    return direction;
}

WasmVector2D WasmRoadSegment::getLanePositionAtDistance(WasmLaneID laneId, float distance) const {
    if (laneId >= m_lanes.size()) {
        return getPointAtDistance(distance); // Return center point as fallback
    }
    
    WasmVector2D point = getPointAtDistance(distance);
    WasmVector2D direction = getDirectionAtDistance(distance);
    
    // Calculate perpendicular direction (normal)
    WasmVector2D normal(-direction.y, direction.x);
    
    // Calculate lane offset from center
    float offset = 0.0f;
    float laneWidth = m_lanes[laneId].getWidth();
    
    // Calculate offset based on lane index
    // Lanes with even indices go to the right, odd indices to the left
    if (laneId % 2 == 0) {
        offset = (laneId / 2) * laneWidth + laneWidth / 2;
    } else {
        offset = -((laneId + 1) / 2) * laneWidth - laneWidth / 2;
    }
    
    // Apply offset
    point.x += normal.x * offset;
    point.y += normal.y * offset;
    
    return point;
}

// WasmIntersection implementation
WasmIntersection::WasmIntersection(WasmIntersectionID id, const WasmVector2D& position)
    : m_id(id)
    , m_position(position)
{
}

void WasmIntersection::connectRoad(WasmRoadSegment* roadSegment, bool isStart) {
    m_connectedRoads.push_back(roadSegment);
    
    // Update the road's reference to this intersection
    if (isStart) {
        roadSegment->m_startIntersection = this;
    } else {
        roadSegment->m_endIntersection = this;
    }
}

void WasmIntersection::defineConnection(WasmRoadSegmentID inRoadId, WasmLaneID inLaneId, 
                                      WasmRoadSegmentID outRoadId, WasmLaneID outLaneId) {
    // Create a unique key for the incoming lane
    std::string key = std::to_string(inRoadId) + "_" + std::to_string(inLaneId);
    
    // Create the connection
    WasmLaneConnection connection;
    connection.roadId = outRoadId;
    connection.laneId = outLaneId;
    
    // Add to the connections map
    if (m_connections.find(key) == m_connections.end()) {
        m_connections[key] = std::vector<WasmLaneConnection>();
    }
    
    m_connections[key].push_back(connection);
}

// WasmRoadNetwork implementation
WasmRoadNetwork::WasmRoadNetwork()
    : m_nextRoadId(0)
    , m_nextIntersectionId(0)
{
}

WasmRoadSegmentID WasmRoadNetwork::createRoadSegment(float startX, float startY, float endX, float endY) {
    WasmRoadSegmentID id = m_nextRoadId++;
    WasmVector2D startPoint(startX, startY);
    WasmVector2D endPoint(endX, endY);
    
    // Create a new road segment
    auto roadSegment = std::make_unique<WasmRoadSegment>(id, startPoint, endPoint);
    
    // Add a default lane
    roadSegment->addLane();
    
    // Add to the road network
    m_roadSegments[id] = std::move(roadSegment);
    
    return id;
}

WasmIntersectionID WasmRoadNetwork::createIntersection(float x, float y) {
    WasmIntersectionID id = m_nextIntersectionId++;
    WasmVector2D position(x, y);
    
    // Create a new intersection
    auto intersection = std::make_unique<WasmIntersection>(id, position);
    
    // Add to the road network
    m_intersections[id] = std::move(intersection);
    
    return id;
}

WasmIntersectionID WasmRoadNetwork::connectWithIntersection(WasmRoadSegmentID road1Id, bool road1End, 
                                                       WasmRoadSegmentID road2Id, bool road2End) {
    // Get the road segments
    WasmRoadSegment* road1 = getRoadSegment(road1Id);
    WasmRoadSegment* road2 = getRoadSegment(road2Id);
    
    if (!road1 || !road2) {
        return static_cast<WasmIntersectionID>(-1); // Invalid ID
    }
    
    // Get the connection points
    const WasmVector2D& p1 = road1End ? road1->getEndPoint() : road1->getStartPoint();
    const WasmVector2D& p2 = road2End ? road2->getEndPoint() : road2->getStartPoint();
    
    // Create an intersection at the midpoint
    WasmVector2D midpoint(
        (p1.x + p2.x) / 2,
        (p1.y + p2.y) / 2
    );
    
    WasmIntersectionID intersectionId = createIntersection(midpoint.x, midpoint.y);
    WasmIntersection* intersection = getIntersection(intersectionId);
    
    // Connect roads to the intersection
    intersection->connectRoad(road1, !road1End);
    intersection->connectRoad(road2, !road2End);
    
    // Define default connections between all lanes
    for (WasmLaneID lane1 = 0; lane1 < road1->getLaneCount(); lane1++) {
        for (WasmLaneID lane2 = 0; lane2 < road2->getLaneCount(); lane2++) {
            intersection->defineConnection(road1Id, lane1, road2Id, lane2);
            intersection->defineConnection(road2Id, lane2, road1Id, lane1);
        }
    }
    
    return intersectionId;
}

WasmRoadSegment* WasmRoadNetwork::getRoadSegment(WasmRoadSegmentID id) const {
    auto it = m_roadSegments.find(id);
    if (it != m_roadSegments.end()) {
        return it->second.get();
    }
    return nullptr;
}

WasmIntersection* WasmRoadNetwork::getIntersection(WasmIntersectionID id) const {
    auto it = m_intersections.find(id);
    if (it != m_intersections.end()) {
        return it->second.get();
    }
    return nullptr;
}

WasmRoadSegmentID WasmRoadNetwork::findNearestRoadSegment(float x, float y, float maxDistance) const {
    WasmVector2D point(x, y);
    float nearestDistance = maxDistance;
    WasmRoadSegmentID nearestId = static_cast<WasmRoadSegmentID>(-1);
    
    for (const auto& [id, road] : m_roadSegments) {
        // Calculate distance from point to line segment
        const WasmVector2D& start = road->getStartPoint();
        const WasmVector2D& end = road->getEndPoint();
        
        // Calculate road vector
        WasmVector2D roadVec(end.x - start.x, end.y - start.y);
        float roadLength = std::sqrt(roadVec.x * roadVec.x + roadVec.y * roadVec.y);
        
        if (roadLength < 0.0001f) {
            // Point-like road, use distance to start point
            float distToStart = std::sqrt((x - start.x) * (x - start.x) + (y - start.y) * (y - start.y));
            if (distToStart < nearestDistance) {
                nearestDistance = distToStart;
                nearestId = id;
            }
            continue;
        }
        
        // Normalize road vector
        WasmVector2D roadDir(roadVec.x / roadLength, roadVec.y / roadLength);
        
        // Vector from road start to point
        WasmVector2D pointVec(x - start.x, y - start.y);
        
        // Project point onto road line
        float projection = pointVec.x * roadDir.x + pointVec.y * roadDir.y;
        
        float distance;
        if (projection < 0) {
            // Point is before start of road
            distance = std::sqrt((x - start.x) * (x - start.x) + (y - start.y) * (y - start.y));
        } else if (projection > roadLength) {
            // Point is after end of road
            distance = std::sqrt((x - end.x) * (x - end.x) + (y - end.y) * (y - end.y));
        } else {
            // Point is alongside road
            WasmVector2D projPoint(
                start.x + roadDir.x * projection,
                start.y + roadDir.y * projection
            );
            distance = std::sqrt((x - projPoint.x) * (x - projPoint.x) + (y - projPoint.y) * (y - projPoint.y));
        }
        
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearestId = id;
        }
    }
    
    return nearestId;
}

WasmIntersectionID WasmRoadNetwork::findNearestIntersection(float x, float y, float maxDistance) const {
    WasmVector2D point(x, y);
    float nearestDistance = maxDistance;
    WasmIntersectionID nearestId = static_cast<WasmIntersectionID>(-1);
    
    for (const auto& [id, intersection] : m_intersections) {
        const WasmVector2D& pos = intersection->getPosition();
        float distance = std::sqrt((x - pos.x) * (x - pos.x) + (y - pos.y) * (y - pos.y));
        
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearestId = id;
        }
    }
    
    return nearestId;
}

bool WasmRoadNetwork::loadFromJSON(const std::string& jsonString) {
    // Simplified JSON parsing for WASM implementation
    // In a real implementation, use a proper JSON library
    // For now, return false indicating failure
    return false;
}

std::string WasmRoadNetwork::exportToJSON() const {
    // Simplified JSON export for WASM implementation
    // In a real implementation, use a proper JSON library
    std::string json = "{ \"roads\": [";
    
    bool first = true;
    for (const auto& [id, road] : m_roadSegments) {
        if (!first) json += ",";
        first = false;
        
        const WasmVector2D& start = road->getStartPoint();
        const WasmVector2D& end = road->getEndPoint();
        
        json += "{ \"id\": " + std::to_string(id);
        json += ", \"startX\": " + std::to_string(start.x);
        json += ", \"startY\": " + std::to_string(start.y);
        json += ", \"endX\": " + std::to_string(end.x);
        json += ", \"endY\": " + std::to_string(end.y);
        json += ", \"lanes\": " + std::to_string(road->getLaneCount());
        json += " }";
    }
    
    json += "], \"intersections\": [";
    
    first = true;
    for (const auto& [id, intersection] : m_intersections) {
        if (!first) json += ",";
        first = false;
        
        const WasmVector2D& pos = intersection->getPosition();
        
        json += "{ \"id\": " + std::to_string(id);
        json += ", \"x\": " + std::to_string(pos.x);
        json += ", \"y\": " + std::to_string(pos.y);
        json += " }";
    }
    
    json += "]}";
    
    return json;
}

void WasmRoadNetwork::clear() {
    m_roadSegments.clear();
    m_intersections.clear();
    m_nextRoadId = 0;
    m_nextIntersectionId = 0;
}

} // namespace amamoto