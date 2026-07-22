#pragma once

#include "engine/core/Resource.h"

#include "raylib.h"
#include "raymath.h"
#include <limits>
#include <vector>

namespace eng {

// Links nodes with edges creating a graph used by A* alongside `InfluenceMap` resources to find
// most optimal path
class SparseGraph : public Resource
{
    ENG_RESOURCE(SparseGraph)

    static constexpr unsigned InvalidNodeIndex = std::numeric_limits<unsigned>::max();

public:
    struct Edge
    {
        unsigned toIndex;   // Target node to move to
        float distanceCost; // 1.0f for non diagonals, otherwise sqrt(2), including to high ground
    };

    struct Node
    {
        static constexpr Vector3 Invalid = Vector3{
            .x = std::numeric_limits<float>::max(),
            .y = std::numeric_limits<float>::max(),
            .z = std::numeric_limits<float>::max(),
        };

        unsigned index  = InvalidNodeIndex; // Tilemap cell index, Will link to influence maps later
        unsigned tileId = InvalidNodeIndex; // Tilemap Tile ID used to query TileDefinition
        Vector3 position = Vector3Zeros;    // Cell world position
        std::vector<Edge> edges;            // All connections to walkable neighboring tiles

        bool Valid() const { return index != InvalidNodeIndex; }
    };

    SparseGraph(unsigned reserve);

    bool Empty() const { return m_Nodes.empty(); }
    bool ContainsNode(unsigned index) { return m_Nodes.at(index).Valid(); }
    unsigned GetNodeCount() const { return m_NodeCount; }
    size_t GetPackedSize() const { return m_Nodes.size(); }

    const Node* GetNode(unsigned index) const;
    const Edge* GetEdge(unsigned fromIndex, unsigned toIndex) const;
    const Node* GetNeighboringNode(unsigned fromIndex, unsigned toIndex) const;
    const Node* FindClosestNodeToPosition(const Vector3& position) const;

    const Node* AddNode(unsigned index, unsigned tileId, const Vector3& position);
    const Edge* AddEdge(unsigned fromIndex, unsigned toIndex, float distanceCost);

    void RemoveNode(unsigned index);
    void RemoveEdge(unsigned fromIndex, unsigned toIndex);
    void PruneNodesWithNoEdges();

    void Clear();

private:
    unsigned m_NodeCount = 0;
    std::vector<Node> m_Nodes; // Use a packed array of nodes over dictionary for performance
};

} // namespace eng
