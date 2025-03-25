#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include "SimpleTrafficSimulation.h"

namespace amamoto {

// Forward declarations
class WasmRoadSegment;
class WasmIntersection;

// Lane ID type
using WasmLaneID = uint32_t;
using WasmRoadSegmentID = uint32_t;
using WasmIntersectionID = uint32_t;

// Lane connection record
struct WasmLaneConnection {
    WasmRoadSegmentID roadId;
    WasmLaneID laneId;
};

// Lane types
enum class WasmLaneType {
    Driving,
    Parking,
    Bike,
    Bus
};

// Lane class for WebAssembly
class WasmLane {
public:
    WasmLane(WasmLaneID id, WasmRoadSegment* parent, float width, WasmLaneType type);
    
    WasmLaneID getId() const { return m_id; }
    float getWidth() const { return m_width; }
    WasmLaneType getType() const { return m_type; }
    
private:
    WasmLaneID m_id;
    WasmRoadSegment* m_parent;
    float m_width;
    WasmLaneType m_type;
    std::vector<WasmVector2D> m_centerLine;
};

// Road segment connecting two points
class WasmRoadSegment {
public:
    WasmRoadSegment(WasmRoadSegmentID id, const WasmVector2D& startPoint, const WasmVector2D& endPoint);
    
    // Get road ID
    WasmRoadSegmentID getId() const { return m_id; }
    
    // Get start and end points
    const WasmVector2D& getStartPoint() const { return m_startPoint; }
    const WasmVector2D& getEndPoint() const { return m_endPoint; }
    
    // Get length
    float getLength() const { return m_length; }
    
    // Get road width (sum of all lanes)
    float getWidth() const;
    
    // Lane management
    WasmLaneID addLane(float width = 3.5f, WasmLaneType type = WasmLaneType::Driving);
    size_t getLaneCount() const { return m_lanes.size(); }
    
    // Get point along the road at a given distance
    WasmVector2D getPointAtDistance(float distance) const;
    
    // Get direction at a given distance
    WasmVector2D getDirectionAtDistance(float distance) const;
    
    // Get lane position at a given distance
    WasmVector2D getLanePositionAtDistance(WasmLaneID laneId, float distance) const;
    
    // Get intersection connections
    WasmIntersection* getStartIntersection() const { return m_startIntersection; }
    WasmIntersection* getEndIntersection() const { return m_endIntersection; }
    
    // For TrafficSimulation to access
    friend class WasmTrafficSimulation;
    friend class WasmIntersection;
    
private:
    WasmRoadSegmentID m_id;
    WasmVector2D m_startPoint;
    WasmVector2D m_endPoint;
    float m_length;
    std::vector<WasmLane> m_lanes;
    WasmIntersection* m_startIntersection;
    WasmIntersection* m_endIntersection;
};

// Intersection connecting multiple road segments
class WasmIntersection {
public:
    WasmIntersection(WasmIntersectionID id, const WasmVector2D& position);
    
    // Get intersection ID
    WasmIntersectionID getId() const { return m_id; }
    
    // Get position
    const WasmVector2D& getPosition() const { return m_position; }
    
    // Connect a road segment to this intersection
    void connectRoad(WasmRoadSegment* roadSegment, bool isStart);
    
    // Define a connection between an incoming lane and outgoing lanes
    void defineConnection(WasmRoadSegmentID inRoadId, WasmLaneID inLaneId, 
                         WasmRoadSegmentID outRoadId, WasmLaneID outLaneId);
    
private:
    WasmIntersectionID m_id;
    WasmVector2D m_position;
    std::vector<WasmRoadSegment*> m_connectedRoads;
    std::unordered_map<std::string, std::vector<WasmLaneConnection>> m_connections;
};

// Road network managing all road segments and intersections
class WasmRoadNetwork {
public:
    WasmRoadNetwork();
    
    // Create a new road segment
    WasmRoadSegmentID createRoadSegment(float startX, float startY, float endX, float endY);
    
    // Create a new intersection
    WasmIntersectionID createIntersection(float x, float y);
    
    // Connect two road segments with an intersection
    WasmIntersectionID connectWithIntersection(WasmRoadSegmentID road1, bool road1End, 
                               WasmRoadSegmentID road2, bool road2End);
    
    // Get a road segment by ID
    WasmRoadSegment* getRoadSegment(WasmRoadSegmentID id) const;
    
    // Get an intersection by ID
    WasmIntersection* getIntersection(WasmIntersectionID id) const;
    
    // Find nearest road segment to a point
    WasmRoadSegmentID findNearestRoadSegment(float x, float y, float maxDistance = 50.0f) const;
    
    // Find nearest intersection to a point
    WasmIntersectionID findNearestIntersection(float x, float y, float maxDistance = 50.0f) const;
    
    // Load from JSON string
    bool loadFromJSON(const std::string& jsonString);
    
    // Export to JSON string
    std::string exportToJSON() const;
    
    // Clear all roads and intersections
    void clear();
    
private:
    std::unordered_map<WasmRoadSegmentID, std::unique_ptr<WasmRoadSegment>> m_roadSegments;
    std::unordered_map<WasmIntersectionID, std::unique_ptr<WasmIntersection>> m_intersections;
    WasmRoadSegmentID m_nextRoadId;
    WasmIntersectionID m_nextIntersectionId;
};

} // namespace amamoto