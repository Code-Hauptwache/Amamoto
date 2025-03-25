#pragma once

#include "Entity.h"
#include "Components.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace amamoto {

// Forward declarations
class RoadSegment;
class Intersection;
class Lane;

// Lane ID type
using LaneID = uint32_t;

// Road Segment ID type
using RoadSegmentID = uint32_t;

// Intersection ID type
using IntersectionID = uint32_t;

// Lane types
enum class LaneType {
    DRIVING,        // Standard driving lane
    PARKING,        // Parking lane
    BICYCLE,        // Bicycle lane
    BUS,            // Bus lane
    EMERGENCY,      // Emergency vehicle lane
    SIDEWALK        // Pedestrian sidewalk
};

// Connection point for connecting road segments and intersections
struct ConnectionPoint {
    Vector2D position;
    Vector2D direction;  // Normalized direction vector
    
    ConnectionPoint() = default;
    ConnectionPoint(const Vector2D& pos, const Vector2D& dir)
        : position(pos), direction(dir) {}
};

// Lane class - represents a single lane within a road segment
class Lane {
public:
    Lane(LaneID id, RoadSegment* parent, float width, LaneType type);
    
    LaneID getID() const { return m_id; }
    float getWidth() const { return m_width; }
    LaneType getType() const { return m_type; }
    
    // Get the center line of the lane as a series of points
    std::vector<Vector2D> getCenterLine() const;
    
    // Get the left and right boundaries of the lane
    std::vector<Vector2D> getLeftBoundary() const;
    std::vector<Vector2D> getRightBoundary() const;
    
    // Check if a vehicle can use this lane
    bool canBeUsedBy(const Entity& vehicle) const;
    
private:
    LaneID m_id;
    RoadSegment* m_parent;
    float m_width;
    LaneType m_type;
    std::vector<Vector2D> m_centerLine;
    
    // Cached boundaries
    mutable std::vector<Vector2D> m_leftBoundary;
    mutable std::vector<Vector2D> m_rightBoundary;
    mutable bool m_boundariesDirty = true;
    
    // Recalculate boundaries if needed
    void updateBoundaries() const;
};

// Road Segment class - represents a section of road between intersections
class RoadSegment {
public:
    RoadSegment(RoadSegmentID id, const ConnectionPoint& start, const ConnectionPoint& end);
    
    RoadSegmentID getID() const { return m_id; }
    
    // Add a lane to the road segment
    LaneID addLane(float width, LaneType type);
    
    // Get a lane by ID
    Lane* getLane(LaneID id);
    
    // Get all lanes
    const std::vector<std::unique_ptr<Lane>>& getLanes() const { return m_lanes; }
    
    // Get connection points
    const ConnectionPoint& getStartPoint() const { return m_startPoint; }
    const ConnectionPoint& getEndPoint() const { return m_endPoint; }
    
    // Get intersections at each end (if any)
    Intersection* getStartIntersection() const { return m_startIntersection; }
    Intersection* getEndIntersection() const { return m_endIntersection; }
    
    // Set intersections
    void setStartIntersection(Intersection* intersection) { m_startIntersection = intersection; }
    void setEndIntersection(Intersection* intersection) { m_endIntersection = intersection; }
    
    // Get length of the road segment
    float getLength() const;
    
    // Get point along the road segment at a given distance
    Vector2D getPointAtDistance(float distance) const;
    
    // Get direction at a given distance
    Vector2D getDirectionAtDistance(float distance) const;
    
private:
    RoadSegmentID m_id;
    ConnectionPoint m_startPoint;
    ConnectionPoint m_endPoint;
    Intersection* m_startIntersection = nullptr;
    Intersection* m_endIntersection = nullptr;
    std::vector<std::unique_ptr<Lane>> m_lanes;
    float m_length;
    
    // Helper to calculate the center line of the road
    std::vector<Vector2D> m_centerLine;
};

// Traffic signal state
enum class SignalState {
    GREEN,
    YELLOW,
    RED
};

// Traffic signal class - manages traffic signal timing
class TrafficSignal {
public:
    TrafficSignal();
    
    // Configure signal timing
    void configure(float greenTime, float yellowTime, float redTime);
    
    // Update the signal state
    void update(float dt);
    
    // Get current state
    SignalState getState() const { return m_currentState; }
    
    // Get time until next state change
    float getTimeUntilChange() const;
    
private:
    float m_greenTime;
    float m_yellowTime;
    float m_redTime;
    float m_cycleTime;
    float m_timer;
    SignalState m_currentState;
};

// Connection between lane and intersection
struct IntersectionConnection {
    RoadSegmentID roadSegmentID;
    LaneID laneID;
    std::vector<std::pair<RoadSegmentID, LaneID>> allowedDestinations;
    TrafficSignal signal;
};

// Intersection class - represents a junction where multiple road segments meet
class Intersection {
public:
    Intersection(IntersectionID id, const Vector2D& position);
    
    IntersectionID getID() const { return m_id; }
    const Vector2D& getPosition() const { return m_position; }
    
    // Connect a road segment to the intersection
    void connectRoadSegment(RoadSegment* roadSegment, bool isStart);
    
    // Disconnect a road segment
    void disconnectRoadSegment(RoadSegmentID id);
    
    // Get all connected road segments
    const std::vector<RoadSegment*>& getConnectedRoadSegments() const { return m_connectedRoadSegments; }
    
    // Define a connection between an incoming and outgoing lane
    void defineConnection(RoadSegmentID inRoadID, LaneID inLaneID, 
                         RoadSegmentID outRoadID, LaneID outLaneID);
    
    // Update all traffic signals
    void updateSignals(float dt);
    
    // Configure signal timing for all signals
    void configureSignalTiming(float greenTime, float yellowTime, float redTime);
    
private:
    IntersectionID m_id;
    Vector2D m_position;
    std::vector<RoadSegment*> m_connectedRoadSegments;
    std::unordered_map<RoadSegmentID, std::vector<IntersectionConnection>> m_connections;
    
    // Make RoadNetwork a friend so it can access private members
    friend class RoadNetwork;
};

// Road Network class - manages all road segments and intersections
class RoadNetwork {
public:
    RoadNetwork();
    ~RoadNetwork();
    
    // Create a new road segment
    RoadSegmentID createRoadSegment(const ConnectionPoint& start, const ConnectionPoint& end);
    
    // Create a new intersection
    IntersectionID createIntersection(const Vector2D& position);
    
    // Connect road segments with an intersection
    void connectWithIntersection(RoadSegmentID road1, bool road1End, 
                               RoadSegmentID road2, bool road2End);
    
    // Get a road segment by ID
    RoadSegment* getRoadSegment(RoadSegmentID id) const;
    
    // Get an intersection by ID
    Intersection* getIntersection(IntersectionID id) const;
    
    // Get all road segments
    const std::unordered_map<RoadSegmentID, std::unique_ptr<RoadSegment>>& getRoadSegments() const { 
        return m_roadSegments; 
    }
    
    // Get all intersections
    const std::unordered_map<IntersectionID, std::unique_ptr<Intersection>>& getIntersections() const { 
        return m_intersections; 
    }
    
    // Find path between two points
    std::vector<std::pair<RoadSegmentID, LaneID>> findPath(
        const Vector2D& start, const Vector2D& end);
    
    // Update the road network (traffic signals, etc.)
    void update(float dt);
    
private:
    std::unordered_map<RoadSegmentID, std::unique_ptr<RoadSegment>> m_roadSegments;
    std::unordered_map<IntersectionID, std::unique_ptr<Intersection>> m_intersections;
    
    RoadSegmentID m_nextRoadSegmentID = 0;
    IntersectionID m_nextIntersectionID = 0;
    LaneID m_nextLaneID = 0;
};

// Vehicle path-following component
class PathFollowingComponent : public Component {
public:
    PathFollowingComponent() = default;
    
    // Set the path to follow
    void setPath(const std::vector<std::pair<RoadSegmentID, LaneID>>& path);
    
    // Update the vehicle based on the path
    void update(float dt, TransformComponent& transform, VehicleComponent& vehicle, const RoadNetwork& roadNetwork);
    
    // Check if reached destination
    bool hasReachedDestination() const { return m_currentPathIndex >= m_path.size(); }
    
private:
    std::vector<std::pair<RoadSegmentID, LaneID>> m_path;
    size_t m_currentPathIndex = 0;
    float m_distanceAlongCurrentSegment = 0.0f;
    
    // Helper function to steer toward target
    Vector2D calculateSteeringForce(const Vector2D& currentPos, const Vector2D& currentVel, 
                                 const Vector2D& targetPos, float maxSpeed);
};

} // namespace amamoto