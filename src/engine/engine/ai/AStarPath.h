#pragma once

#include "engine/ai/InfluenceMap.h"
#include "engine/ai/SparseGraph.h"

#include <limits>
#include <vector>

namespace eng {

class RenderQueue;

enum class AStarHCostType
{
    Manhattan,  // Faster calculation, best for 4-way grid movement
    Hypotenuse, // Slower (uses sqrt), better for accurate or 8-way movement
};

class AStarPath
{
    struct Node
    {
        unsigned index; // Maps to the tilemap index
        float cost;     // Directional cost

        bool operator>(const Node& other) const;
    };

    struct KnownNode
    {
        static constexpr float NotVisited = std::numeric_limits<float>::max();

        float accumulativeGCost = NotVisited; // added onto gCost
        unsigned parentIndex    = NotVisited; // Parent node
        bool closed             = false;      // avoid visiting again

        bool IsKnown() const { return accumulativeGCost != NotVisited; }
    };

public:
    static constexpr unsigned InvalidIndex  = std::numeric_limits<unsigned>::max();
    static constexpr unsigned InfiniteSteps = std::numeric_limits<unsigned>::max();

    AStarPath(AStarHCostType hCostType);

    // Binds the graph used for pathfinding. Must not be null
    void Initialize(SparseGraph* graph);

    unsigned GetCurrentIndex() const;
    const SparseGraph::Node* GetCurrentNode() const; // Returns nullptr if path is empty
    const SparseGraph::Node* GetNextNode() const;    // Returns nullptr if at the end of the path
    unsigned GetPathBegin() const; // Returns InvalidIndex if path has not been set
    unsigned GetPathEnd() const;   // Returns InvalidIndex if path has not been set
    unsigned GetPathNodeCount() const;
    bool EmptyPath() const;
    const std::vector<unsigned>* GetFullRoute() const; // Returns nullptr if no path generated

    // Advances or retreats the internal iterator along the calculated route
    void SetNextPathIndex();
    void SetPreviousPathIndex();

    // Renders the path from the *current* index to the end
    void RenderPath(RenderQueue& renderQueue, Color color = RED,
                    unsigned maxSteps = InfiniteSteps) const;
    // Renders the entire generated path, ignoring the current index
    void DebugRenderFullPath(Color color = RED) const;

    // Calculates optimal path using A*. Returns true if a valid route to endIndex is found
    bool GeneratePath(unsigned beginIndex, unsigned endIndex,
                      const std::vector<InfluenceMap*>& influenceMaps, unsigned maxSteps);

    // Truncates the currently generated path if it exceeds the specified node limit
    void PruneToMaxSteps(unsigned maxSteps);

private:
    bool PlanRoute(const std::vector<KnownNode>& route, unsigned beginIndex, unsigned endIndex);

    float GetHCost(unsigned currentIndex, unsigned targetIndex) const;
    float GetManhattanCost(unsigned currentIndex, unsigned targetIndex) const;
    float GetHypotenuseCost(unsigned currentIndex, unsigned targetIndex) const;

    // Calculates the traversal cost between adjacent nodes. Modifies the base edge distance
    // using Influence Maps to allow for tactical pathing (e.g., hazard avoidance or position
    // advantages).
    float GetGCost(const std::vector<InfluenceMap*>& influenceMaps, unsigned currentIndex,
                   unsigned destIndex);

    SparseGraph* m_Graph;             // Graph used for node and edge lookups
    unsigned m_CurrentRouteIndex = 0; // Current iterator position in the calculated route
    AStarHCostType m_HCostType;       // Heuristic calculation method
    std::vector<unsigned> m_Route;    // The generated path sequence of node indices as packed data
};

} // namespace eng
