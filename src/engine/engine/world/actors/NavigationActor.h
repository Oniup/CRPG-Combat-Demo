#pragma once

#include "engine/ai/AStarPath.h"
#include "engine/ai/InfluenceMap.h"
#include "engine/ai/SparseGraph.h"
#include "engine/world/actors/Actor.h"
#include <vector>

namespace eng {

class RenderQueue;

// Implements A* to move Actor around the scene based on a Sparse Graph and Influence Maps
class NavigationActor : public Actor
{
public:
    // Parse SparseGraph and InfluenceMaps that A* will use to calculate paths.
    void InitializeNavigation(SparseGraph* graph, std::vector<InfluenceMap*>&& influenceMaps);

    // Sets the target destination to specified tilemap index and calculates a path using A*.
    virtual bool SetDestination(unsigned index, unsigned maxSteps = AStarPath::InfiniteSteps);

    // Moves the Actor between points defined by A*, following the route. Must Call
    // `SetDestination()` before calling this method otherwise nothing will happen.
    void TraversePath();

    // Returns true when the actor has reached the end of the path.
    bool ReachedEndOfPath();

    // Returns the length of the path in node count
    unsigned GetPathNodeCount() const;
    // Returns the full path that A* has generated. Can be null if no path has been generated
    const std::vector<unsigned>* GetFullRoute() const;

protected:
    NavigationActor(const Vector3& position, float pathBoundsCheckRadius,
                    AStarHCostType hCostType = AStarHCostType::Manhattan);

    // Returns a direction vector pointing to the next target position if `TraversePath()` has been
    // called beforehand, enabling this method. Otherwise it will return a Vector3Zero.
    Vector3 GetPathMoveDirection();

    Vector3 GetPathNodePosition();

    // Once reached to the target node (using `ReachedPathNode()`) this increments the count to the
    // next target position. If reached the end it will stay the same
    void IncrementPathNode();

    // Returns true when the Actor has reached the current node when traversing the path based on a
    // path bounds check radius set through the constructor.
    bool ReachedPathNode();

    // Call within OnRender(), draws a line from the current node the actor is at to the end
    void RenderPath(RenderQueue& renderQueue, Color color,
                    unsigned maxSteps = AStarPath::InfiniteSteps);

    // Shortens the path A* has generated to a max steps if the route node count exceeds that value.
    void PrunePathToMaxSteps(unsigned maxSteps);

    void ApplyForce(float deltaTime);

private:
    AStarPath m_AStar;             // To calculate paths to follow and move around the map
    bool m_TraversePath;           // Tells Actor to follow path
    float m_PathBoundsCheckRadius; // Distance required to increment current node
    std::vector<InfluenceMap*> m_InfluenceMaps; // Influence maps to modify A* result
};

} // namespace eng
