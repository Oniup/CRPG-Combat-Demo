#include "engine/ai/AStarPath.h"

#include "engine/ai/InfluenceMap.h"
#include "engine/ai/SparseGraph.h"
#include "engine/core/Error.h"
#include "engine/graphics/RenderQueue.h"

#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <queue>

namespace eng {

bool AStarPath::Node::operator>(const Node& other) const
{
    if (cost == other.cost)
        return index > other.index;
    return cost > other.cost;
}

AStarPath::AStarPath(AStarHCostType hCostType)
    : m_Graph(nullptr),
      m_HCostType(hCostType)

{
}

void AStarPath::Initialize(SparseGraph* graph)
{
    DASSERT(graph, "Cannot set the graph to a nullptr");
    m_Graph = graph;
}

unsigned AStarPath::GetCurrentIndex() const
{
    if (m_Route.empty())
        return InvalidIndex;
    return m_Route[m_CurrentRouteIndex];
}

const SparseGraph::Node* AStarPath::GetCurrentNode() const
{
    if (m_Route.empty())
        return nullptr;
    return m_Graph->GetNode(m_Route[m_CurrentRouteIndex]);
}

const SparseGraph::Node* AStarPath::GetNextNode() const
{
    if (m_Route.empty() || m_CurrentRouteIndex >= m_Route.size())
        return nullptr;
    return m_Graph->GetNode(m_Route[m_CurrentRouteIndex + 1]);
}

unsigned AStarPath::GetPathBegin() const
{
    if (m_Route.empty())
        return InvalidIndex;
    return m_Route.front();
}

unsigned AStarPath::GetPathEnd() const
{
    if (m_Route.empty())
        return InvalidIndex;
    return m_Route.back();
}

unsigned AStarPath::GetPathNodeCount() const
{
    if (m_Route.empty())
        return 0;
    return m_Route.size();
}

bool AStarPath::EmptyPath() const
{
    return m_Route.empty();
}

const std::vector<unsigned>* AStarPath::GetFullRoute() const
{
    if (m_Route.empty())
        return nullptr;
    return &m_Route;
}

void AStarPath::SetNextPathIndex()
{
    if (m_CurrentRouteIndex < m_Route.size())
        m_CurrentRouteIndex++;
}

void AStarPath::SetPreviousPathIndex()
{
    if (m_CurrentRouteIndex > 0)
        m_CurrentRouteIndex--;
}

void AStarPath::RenderPath(RenderQueue& renderQueue, Color color, unsigned maxSteps) const
{
    if (!EmptyPath() && m_CurrentRouteIndex != GetPathEnd())
    {
        for (unsigned i = m_CurrentRouteIndex; i < m_Route.size() - 1; i++)
        {
            const SparseGraph::Node* currentNode = m_Graph->GetNode(m_Route[i]);
            const SparseGraph::Node* nextNode    = m_Graph->GetNode(m_Route[i + 1]);
            DASSERT(currentNode && nextNode,
                    "Failed to find current node or next node for indexes: Current: {}, Next: {}",
                    m_Route[i],
                    m_Route[i + 1]);

            renderQueue.Submit(
                DrawCommand{
                    .shapeFn =
                        [currentNode, nextNode](DrawCommand& cmd)
                    {
                        DrawCylinderEx(
                            currentNode->position, nextNode->position, 0.1f, 0.1f, 2, cmd.color);
                    },
                    .position = currentNode->position,
                    .color    = color,
                },
                true);
            // DrawCylinderEx(currentNode->position, nextNode->position, 0.1f, 0.1f, 2, color);
        }
    }
}

void AStarPath::DebugRenderFullPath(Color color) const
{
    if (!EmptyPath())
    {
        unsigned prevIndex = m_Route.front();
        for (unsigned i = prevIndex + 1; i < m_Route.size(); i++)
        {
            const SparseGraph::Node* currentNode = m_Graph->GetNode(m_Route[i]);
            const SparseGraph::Node* prevNode    = m_Graph->GetNode(prevIndex);
            prevIndex                            = m_Route[i];

            DrawCylinderEx(prevNode->position, currentNode->position, 0.1f, 0.1f, 2, color);
        }
    }
}

bool AStarPath::GeneratePath(unsigned beginIndex, unsigned endIndex,
                             const std::vector<InfluenceMap*>& influenceMaps, unsigned maxSteps)
{
    DASSERT(m_Graph, "Graph not set!");
    m_CurrentRouteIndex = 0;
    m_Route.clear();

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open; // Not visited nodes
    std::vector<KnownNode> knownNodes(m_Graph->GetPackedSize());           // Known nodes that exist

    // Start with the beginning node
    open.emplace(Node{beginIndex, GetHCost(beginIndex, endIndex)});
    knownNodes[beginIndex] = KnownNode{
        .accumulativeGCost = 0.0f,
        .parentIndex       = beginIndex,
    };

    unsigned stepCount = 0;
    while (!open.empty())
    {
        if (stepCount >= maxSteps)
        {
            ERROR("Failed to generate path beginning at {} to {}, Ran out of steps, max: {}",
                  beginIndex,
                  endIndex,
                  maxSteps);
            return false;
        }
        stepCount++;

        // Get current node
        Node current = open.top();
        open.pop();

        // Skip if already searched through this node
        if (knownNodes[current.index].closed)
            continue;
        knownNodes[current.index].closed = true;

        if (current.index == endIndex)
            break; // Found end path, need to plan route

        // Iterate over all edges
        for (const SparseGraph::Edge& edge : m_Graph->GetNode(current.index)->edges)
        {
            if (!knownNodes[edge.toIndex].closed)
            {
                float gCost = knownNodes[current.index].accumulativeGCost +
                              GetGCost(influenceMaps, current.index, edge.toIndex);
                float hCost = GetHCost(edge.toIndex, endIndex);
                float fCost = hCost + gCost;

                // Skip if there is no benefit to pursue, not pointing in correct direction
                if (gCost >= knownNodes[edge.toIndex].accumulativeGCost)
                    continue;

                // Update destination node's path to the cheaper version
                open.emplace(Node{edge.toIndex, fCost});
                knownNodes[edge.toIndex].accumulativeGCost = gCost;
                knownNodes[edge.toIndex].parentIndex       = current.index;
            }
        }
    }

    return PlanRoute(knownNodes, beginIndex, endIndex);
}

void AStarPath::PruneToMaxSteps(unsigned maxSteps)
{
    if (m_Route.size() > maxSteps)
        m_Route.resize(maxSteps);
}

bool AStarPath::PlanRoute(const std::vector<KnownNode>& route, unsigned beginIndex,
                          unsigned endIndex)
{
    // Failed to find path
    if (!route.at(endIndex).IsKnown() || route.empty())
        return false;

    unsigned currentIndex = endIndex;
    while (currentIndex != beginIndex)
    {
        m_Route.push_back(currentIndex);
        currentIndex = route.at(currentIndex).parentIndex;
    }
    m_Route.push_back(currentIndex); // Add the starting point to the route

    std::reverse(m_Route.begin(), m_Route.end());
    return true;
}

float AStarPath::GetHCost(unsigned currentIndex, unsigned targetIndex) const
{
    switch (m_HCostType)
    {
    case AStarHCostType::Manhattan:  return GetManhattanCost(currentIndex, targetIndex);
    case AStarHCostType::Hypotenuse: return GetHypotenuseCost(currentIndex, targetIndex);
    }
}

float AStarPath::GetManhattanCost(unsigned currentIndex, unsigned targetIndex) const
{
    const SparseGraph::Node* currentNode = m_Graph->GetNode(currentIndex);
    const SparseGraph::Node* targetNode  = m_Graph->GetNode(targetIndex);
    DASSERT(currentNode, "Current node at index {} was not added to the SparseGraph", currentIndex);
    DASSERT(targetNode, "Target node at index {} was not added to the SparseGraph", currentIndex);

    Vector3 direction = targetNode->position - currentNode->position;
    return std::abs(direction.x) + std::abs(direction.y) + std::abs(direction.z);
}

float AStarPath::GetHypotenuseCost(unsigned currentIndex, unsigned targetIndex) const
{
    const SparseGraph::Node* currentNode = m_Graph->GetNode(currentIndex);
    const SparseGraph::Node* targetNode  = m_Graph->GetNode(targetIndex);
    DASSERT(currentNode, "Current node at index {} was not added to the SparseGraph", currentIndex);
    DASSERT(targetNode, "Target node at index {} was not added to the SparseGraph", currentIndex);

    Vector3 direction = targetNode->position - currentNode->position;
    return Vector3Length(direction); // Square root version, slower than Manhattan but accurate
}

float AStarPath::GetGCost(const std::vector<InfluenceMap*>& influenceMaps, unsigned fromIndex,
                          unsigned toIndex)
{
    float gCost = m_Graph->GetEdge(fromIndex, toIndex)->distanceCost;
    for (const InfluenceMap* influence : influenceMaps)
        influence->ApplyInfluence(gCost, toIndex);
    return gCost;
}

} // namespace eng
