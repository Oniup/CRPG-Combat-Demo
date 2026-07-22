#include "engine/world/actors/NavigationActor.h"

#include "engine/ai/SparseGraph.h"
#include "engine/world/World.h"
#include "engine/world/actors/Tilemap.h"
#include "raylib.h"
#include "raymath.h"

namespace eng {

void NavigationActor::InitializeNavigation(SparseGraph* graph,
                                           std::vector<InfluenceMap*>&& influenceMaps)
{
    m_AStar.Initialize(graph);
    m_InfluenceMaps = std::move(influenceMaps);

    const SparseGraph::Node* node = graph->FindClosestNodeToPosition(position);
    position                      = node->position;
}

bool NavigationActor::SetDestination(unsigned index, unsigned maxSteps)
{
    if (index == m_AStar.GetPathEnd())
        return true;

    unsigned beginIndex = m_AStar.GetCurrentIndex() == m_AStar.GetPathEnd()
                              ? m_AStar.GetPathEnd()
                              : m_AStar.GetCurrentIndex();

    if (beginIndex == AStarPath::InvalidIndex) // Only will need to use this method for first path
    {
        Tilemap* tilemap = GetWorld()->GetTilemap();
        beginIndex       = tilemap->GetIndexFromPosition(static_cast<int>(position.x),
                                                         static_cast<int>(position.y),
                                                         static_cast<int>(position.z));
    }
    return m_AStar.GeneratePath(beginIndex, index, m_InfluenceMaps, maxSteps);
}

void NavigationActor::TraversePath()
{
    m_TraversePath = true;
}

bool NavigationActor::ReachedEndOfPath()
{
    return m_AStar.GetCurrentIndex() == m_AStar.GetPathEnd();
}

unsigned NavigationActor::GetPathNodeCount() const
{
    return m_AStar.GetPathNodeCount();
}

const std::vector<unsigned>* NavigationActor::GetFullRoute() const
{
    return m_AStar.GetFullRoute();
}

NavigationActor::NavigationActor(const Vector3& position, float pathBoundsCheckRadius,
                                 AStarHCostType hCostType)
    : Actor(position),
      m_AStar(hCostType),
      m_PathBoundsCheckRadius(pathBoundsCheckRadius)
{
}

Vector3 NavigationActor::GetPathMoveDirection()
{
    if (m_AStar.EmptyPath())
    {
        m_TraversePath = false;
        return SparseGraph::Node::Invalid;
    }

    if (!m_TraversePath || m_AStar.GetCurrentIndex() == m_AStar.GetPathEnd())
    {
        m_TraversePath = false;
        return Vector3Zeros;
    }

    const SparseGraph::Node* nextNode = m_AStar.GetNextNode();
    return Vector3Normalize(nextNode->position - position);
}

Vector3 NavigationActor::GetPathNodePosition()
{
    if (m_AStar.EmptyPath())
    {
        m_TraversePath = false;
        return SparseGraph::Node::Invalid;
    }

    if (!m_TraversePath || m_AStar.GetCurrentIndex() == m_AStar.GetPathEnd())
    {
        m_TraversePath = false;
        return m_AStar.GetCurrentNode()->position;
    }

    const SparseGraph::Node* nextNode = m_AStar.GetNextNode();
    return nextNode->position;
}

void NavigationActor::IncrementPathNode()
{
    DASSERT(m_AStar.EmptyPath() || m_AStar.GetCurrentIndex() != m_AStar.GetPathEnd(),
            "Must check if path exists before calling this function");
    m_AStar.SetNextPathIndex();
}

bool NavigationActor::ReachedPathNode()
{
    DASSERT(m_AStar.EmptyPath() || m_AStar.GetCurrentIndex() != m_AStar.GetPathEnd(),
            "Must check if path exists before calling this function");

    const SparseGraph::Node* nextNode = m_AStar.GetNextNode();
    Vector3 direction                 = nextNode->position - position;
    float lengthSq                    = Vector3LengthSqr(direction);
    return lengthSq < m_PathBoundsCheckRadius;
}

void NavigationActor::RenderPath(RenderQueue& renderQueue, Color color, unsigned maxSteps)
{
    m_AStar.RenderPath(renderQueue, color, maxSteps);
}

void NavigationActor::PrunePathToMaxSteps(unsigned maxSteps)
{
    m_AStar.PruneToMaxSteps(maxSteps);
}

} // namespace eng
