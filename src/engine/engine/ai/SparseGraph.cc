#include "engine/ai/SparseGraph.h"
#include "engine/core/Error.h"

#include "raymath.h"
#include <limits>

namespace eng {

SparseGraph::SparseGraph(unsigned reserve)
    : m_Nodes(reserve)
{
}

const SparseGraph::Node* SparseGraph::GetNode(unsigned index) const
{
    if (!m_Nodes.at(index).Valid())
        return nullptr;
    return &m_Nodes.at(index);
}

const SparseGraph::Edge* SparseGraph::GetEdge(unsigned fromIndex, unsigned toIndex) const
{
    DASSERT(m_Nodes.at(fromIndex).Valid());

    const std::vector<Edge>& edges = m_Nodes.at(fromIndex).edges;
    for (const Edge& edge : edges)
        if (edge.toIndex == toIndex)
            return &edge;

    return nullptr;
}

const SparseGraph::Node* SparseGraph::GetNeighboringNode(unsigned fromIndex, unsigned toIndex) const
{
    const Edge* edge = GetEdge(fromIndex, toIndex);
    if (edge)
        return &m_Nodes.at(edge->toIndex);
    return nullptr;
}

const SparseGraph::Node* SparseGraph::FindClosestNodeToPosition(const Vector3& position) const
{
    const Node* closest = nullptr;
    float distanceSqr   = std::numeric_limits<float>::max();
    for (const Node& node : m_Nodes)
    {
        Vector3 direction = node.position - position;
        float lengthSqr   = Vector3Length(direction);
        if (lengthSqr < distanceSqr)
        {
            closest     = &node;
            distanceSqr = lengthSqr;
        }
    }
    return closest;
}

const SparseGraph::Node* SparseGraph::AddNode(unsigned index, unsigned tileId,
                                              const Vector3& position)
{
    if (m_Nodes.at(index).Valid())
        return &m_Nodes.at(index);

    m_NodeCount++;
    m_Nodes[index] = Node{index, tileId, position};
    return &m_Nodes.at(index);
}

const SparseGraph::Edge* SparseGraph::AddEdge(unsigned fromIndex, unsigned toIndex,
                                              float distanceCost)
{
    DASSERT(m_Nodes.at(fromIndex).Valid(), "from Node with index {} has not been added", fromIndex);

    m_Nodes[fromIndex].edges.push_back(Edge{toIndex, distanceCost});
    return &m_Nodes.at(fromIndex).edges.back();
}

void SparseGraph::RemoveNode(unsigned index)
{
    if (!m_Nodes.at(index).Valid())
        return;

    // Remove neighboring edges that link directly to removing node
    for (const Edge& edge : m_Nodes[index].edges)
        RemoveEdge(edge.toIndex, index);

    // Remove node from sparse graph
    m_Nodes.at(index).index = InvalidNodeIndex;
    m_Nodes.at(index).edges.clear();
}

void SparseGraph::RemoveEdge(unsigned fromIndex, unsigned toIndex)
{
    if (!m_Nodes.at(fromIndex).Valid())
        return;

    // Remove from edge list if exists
    std::vector<Edge>& edges = m_Nodes[fromIndex].edges;
    for (size_t i = 0; i < edges.size(); i++)
    {
        if (edges[i].toIndex == toIndex)
        {
            edges.erase(edges.begin() + i);
            return;
        }
    }
}

void SparseGraph::PruneNodesWithNoEdges()
{
    for (Node& node : m_Nodes)
    {
        if (!node.Valid() || !node.edges.empty())
            continue;

        node.index = InvalidNodeIndex;
        m_NodeCount--;
    }
}

void SparseGraph::Clear()
{
    m_Nodes.clear();
}

} // namespace eng
