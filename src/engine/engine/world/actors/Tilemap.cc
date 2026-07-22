#include "engine/world/actors/Tilemap.h"

#include "engine/ai/InfluenceMap.h"
#include "engine/ai/SparseGraph.h"
#include "engine/core/Error.h"
#include "engine/core/ResourceManager.h"
#include "engine/graphics/Model.h"
#include "engine/graphics/RenderQueue.h"
#include "engine/utilities/Color.h"
#include "engine/world/LevelData.h"
#include "engine/world/World.h"
#include "engine/world/actors/Actor.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <limits>

namespace eng {

static constexpr float Sqrt2 = 1.41421356237f; // diagonal distance to prevent zig-zagging

Vector3 TileDefinition::GetPositionOffset(int x, int y, int z) const
{
    Vector3 scaleOffset = (Vector3Ones - size) * 0.5f;
    return Vector3(x, y, z) - scaleOffset;
}

void TileDefinition::GetSlopeNeighboringTiles(int currX, int currY, int currZ, Vector3& low,
                                              Vector3& high) const
{
    switch (slope)
    {
    case 'n':
        low  = Vector3(currX, currY, currZ + 1);
        high = Vector3(currX, currY + 1, currZ - 1);
        break;
    case 's':
        low  = Vector3(currX, currY, currZ - 1);
        high = Vector3(currX, currY + 1, currZ + 1);
        break;
    case 'e':
        low  = Vector3(currX + 1, currY, currZ);
        high = Vector3(currX - 1, currY + 1, currZ);
        break;
    case 'w':
        low  = Vector3(currX - 1, currY, currZ);
        high = Vector3(currX + 1, currY + 1, currZ);
        break;
    default:
        low  = Vector3Zeros;
        high = Vector3Zeros;
        break;
    }
}

Tilemap::Tilemap(const Vector3& position, std::string&& tag)
    : Actor(position, std::move(tag))
{
}

void Tilemap::LoadFromLevelData(const LevelData& levelData, ResourceManager& resources)
{
    m_Width      = levelData.width;
    m_Height     = levelData.layers.size();
    m_Length     = levelData.length;
    m_CellWidth  = levelData.cellWidth;
    m_CellHeight = levelData.cellHeight;

    LoadTileDefinitions(levelData);
    LoadTilesFromLayer(levelData);
    SetupNavGraph(resources);
}

unsigned Tilemap::GetCellTileID(unsigned index) const
{
    return m_Cells[index];
}

const TileDefinition* Tilemap::GetTileDefinition(unsigned id) const
{
    if (m_TileDefs.contains(id))
        return &m_TileDefs.at(id);
    return nullptr;
}

const TileDefinition* Tilemap::GetTileDefinition(int x, int y, int z) const
{
    if (x >= m_Width || y >= m_Height || z >= m_Length)
        return nullptr;

    unsigned index = GetIndexFromPosition(x, y, z);
    unsigned id    = m_Cells[index];
    return GetTileDefinition(id);
}

int Tilemap::GetIndexFromPosition(int x, int y, int z) const
{
    return (y * m_Width * m_Length) + (z * m_Width) + x;
}

void Tilemap::GetPositionFromIndex(unsigned index, int& x, int& y, int& z) const
{
    x = index % m_Width;
    z = (index / m_Width) % m_Length;
    y = index / (m_Width * m_Length);
}

void Tilemap::ConvertVectorToPosition(Vector3 pos, int& x, int& y, int& z) const
{
    x = static_cast<int>(std::round(pos.x));
    y = static_cast<int>(std::round(pos.y));
    z = static_cast<int>(std::round(pos.z));
}

bool Tilemap::IsValidIndex(unsigned index) const
{
    return index < m_Width * m_Height * m_Length;
}

void Tilemap::SubmitDrawCommands(RenderQueue& renderQueue)
{
    IterateCells(
        [&](const TileIterCellContext& cell)
        {
            if (!cell.def)
                return;

            // Set position and scale
            DrawCommand cmd{
                .model    = m_TileModel,
                .position = cell.def->GetPositionOffset(cell.x, cell.y, cell.z),
                .scale    = cell.def->size,
            };

            // Body
            cmd.color = cell.def->tint;
            renderQueue.Submit(cmd);

            if (cell.def->slope != '\0')
            {
                Vector3 low, high;
                cell.def->GetSlopeNeighboringTiles(cell.x, cell.y, cell.z, low, high);

                if (GetWorld()->ShouldRenderDebugInfo())
                {
                    // Debug lower wireframe box
                    cmd.position = low;
                    cmd.color    = RED;
                    cmd.wired    = true;
                    renderQueue.Submit(cmd);

                    // Debug higher wireframe box
                    cmd.position = high;
                    renderQueue.Submit(cmd);
                }
            }

            // Render Navigation graph
            if (GetWorld()->ShouldRenderDebugInfo())
            {
                if (!m_NavGraph->ContainsNode(cell.index))
                    return;

                const SparseGraph::Node* currentNode = m_NavGraph->GetNode(cell.index);
                if (currentNode == nullptr)
                    return;

                // Draw node
                Vector3 drawOffset = Vector3UnitY * (cmd.scale.y * 0.5f);
                renderQueue.Submit(DrawCommand{
                    .shapeFn =
                        [](const DrawCommand& cmd)
                    {
                        DrawCubeWiresV(cmd.position, cmd.scale, cmd.color);
                    },
                    .position = currentNode->position + drawOffset,
                    .scale    = Vector3Ones * 0.1f,
                    .color    = GREEN,
                });

                // Draw Edges
                for (const SparseGraph::Edge& edge : currentNode->edges)
                {
                    const SparseGraph::Node* toNode = m_NavGraph->GetNode(edge.toIndex);
                    renderQueue.Submit(DrawCommand{
                        .shapeFn =
                            [toNode](const DrawCommand& cmd)
                        {
                            DrawLine3D(cmd.position, toNode->position, cmd.color);
                        },
                        .position = currentNode->position + drawOffset,
                        .scale    = cmd.scale,
                        .color    = GREEN,
                    });
                }
            }
        });
}

RayCollision Tilemap::CheckRayCollision(const Ray& ray) const
{
    RayCollision collision{
        .distance = std::numeric_limits<float>::max(),
    };

    IterateCells(
        [&](const TileIterCellContext& cell)
        {
            if (!cell.def->isWalkable)
                return;

            // Check if ray collides with tile
            Vector3 position = cell.def->GetPositionOffset(cell.x, cell.y, cell.z);
            BoundingBox box  = {
                .min = position - cell.def->size * 0.5f,
                .max = position + cell.def->size * 0.5f,

            };
            RayCollision rayCollision = GetRayCollisionBox(ray, box);

            // Only track the closest tile
            if (rayCollision.hit && rayCollision.distance < collision.distance)
                collision = rayCollision;
        });
    return collision;
}

void Tilemap::IterateCells(Pfn_TilemapIterateCells atCellFunc) const
{
    ASSERT(atCellFunc, "atCellFunc must be defined");
    for (int y = 0; y < m_Height; y++)
    {
        for (int z = 0; z < m_Length; z++)
        {
            for (int x = 0; x < m_Width; x++)
            {
                unsigned index            = GetIndexFromPosition(x, y, z);
                unsigned id               = GetCellTileID(index);
                const TileDefinition* def = GetTileDefinition(id);
                if (!def)
                    continue;

                atCellFunc(TileIterCellContext{
                    .x     = x,
                    .y     = y,
                    .z     = z,
                    .index = index,
                    .id    = id,
                    .def   = def,
                });
            }
        }
    }
}

void Tilemap::SetModel(Model* model)
{
    m_TileModel = model;
}

void Tilemap::LoadTileDefinitions(const LevelData& levelData)
{
    for (const auto& [id, def] : levelData.tileDefinitions)
    {
        Vector3 size{
            .x = def.GetField<float>("width", 1.0f),
            .y = def.GetField<float>("height", 1.0f),
            .z = def.GetField<float>("length", 1.0f),
        };
        Vector3 offset{
            .x = def.GetField<float>("offsetX", 0.0f),
            .y = def.GetField<float>("offsetY", 0.0f),
            .z = def.GetField<float>("offsetZ", 0.0f),
        };

        const std::string* color = def.TryGetField<std::string>("tint");

        TileDefinition entry{
            .size       = size,
            .offset     = offset,
            .tint       = ConvertHexToColor(color ? *color : "ffffff"),
            .isWalkable = def.GetField<bool>("walkable", false),
            .baseCost   = def.GetField<int>("cost", 1),
            .slope      = def.GetField<char>("slope", '\0'),
        };

#ifndef NDEBUG
        strncpy_s(entry.nameBuffer, def.name.c_str(), def.name.size());
#endif

        m_TileDefs.emplace(id, entry);
    }
}

void Tilemap::LoadTilesFromLayer(const LevelData& levelData)
{
    m_Cells.reserve(m_Width * m_Height * m_Length);
    for (const Layer& layer : levelData.layers)
    {
        for (unsigned id : layer.terrain)
        {
            m_Cells.push_back(id);
            m_NonAirCellCount++;
        }
    }
}

void Tilemap::SetupNavGraph(ResourceManager& resources)
{
    // Load resources
    m_NavGraph = resources.Load<SparseGraph>("Navigation Graph", m_NonAirCellCount);

    auto* tileWeights = // Guide AI to avoid tiles that limit the max distance when moving
        resources.Load<InfluenceMap>(InfluenceMapName_TileWeight, m_Width, m_Height, m_Length);
    auto* highGroundMap = // Guide AI to high ground
        resources.Load<InfluenceMap>(InfluenceMapName_HighGround,
                                     m_Width,
                                     m_Height,
                                     m_Length,
                                     InfluenceModificationType::Subtract);

    // Setup nav graph nodes and apply static influence map values
    IterateCells(
        [&](const TileIterCellContext& cell)
        {
            if (!cell.def->isWalkable)
                return;

            // Add offset so node snaps to the top face
            Vector3 position = cell.def->GetPositionOffset(cell.x, cell.y, cell.z);
            position.y += cell.def->size.y * 0.5f;
            m_NavGraph->AddNode(cell.index, cell.id, position);

            // Apply influence
            highGroundMap->SetSingleCellInfluence(cell.index, cell.y);
            tileWeights->SetSingleCellInfluence(cell.index, cell.def->baseCost);
        });

    // Construct connections between walkable tiles
    IterateCells(
        [&](const TileIterCellContext& cell)
        {
            if (!cell.def->isWalkable)
                return;

            // Link nodes in direction (low and high ground nodes), skip the x, z axis if slope
            if (AddEdgeFromSlopeToGraph(cell.def, cell.index))
                return;

            // Link edges to other walkable nodes
            AddEdgeToNavGraph(cell.index, cell.y, cell.x + 1, cell.z);           // Right
            AddEdgeToNavGraph(cell.index, cell.y, cell.x - 1, cell.z);           // Left
            AddEdgeToNavGraph(cell.index, cell.y, cell.x, cell.z + 1);           // Forward
            AddEdgeToNavGraph(cell.index, cell.y, cell.x, cell.z - 1);           // Backward
            AddEdgeToNavGraph(cell.index, cell.y, cell.x + 1, cell.z + 1, true); // Right Forward
            AddEdgeToNavGraph(cell.index, cell.y, cell.x - 1, cell.z + 1, true); // Left Forward
            AddEdgeToNavGraph(cell.index, cell.y, cell.x + 1, cell.z - 1, true); // Right Backward
            AddEdgeToNavGraph(cell.index, cell.y, cell.x - 1, cell.z - 1, true); // Left Backward
        });

    m_NavGraph->PruneNodesWithNoEdges();
}

void Tilemap::AddEdgeToNavGraph(unsigned fromIndex, int fromY, int toX, int toZ, bool isDiagonal)
{
    Vector3 low, high;

    // Check bounds
    if (toX < 0 || toX >= m_Width)
        return;
    if (toZ < 0 || toZ >= m_Length)
        return;

    // Add edge if neighboring tile is walkable
    unsigned toIndex = GetIndexFromPosition(toX, fromY, toZ);
    if (!IsValidIndex(toIndex))
        return;

    const TileDefinition* toDef = GetTileDefinition(m_Cells[toIndex]);
    if (toDef && toDef->isWalkable)
    {
        if (toDef->slope == '\0') // Not a slope
        {
            m_NavGraph->AddEdge(fromIndex, toIndex, isDiagonal ? Sqrt2 : 1.0f);
            return;
        }

        // Is a slope? Must connect with the direction of the slope, can't enter the stairs from the
        // side but only from the base of the stairs.
        toDef->GetSlopeNeighboringTiles(toX, fromY, toZ, low, high);
        unsigned lowIndex = GetIndexFromPosition(
            static_cast<int>(low.x), static_cast<int>(low.y), static_cast<int>(low.z));
        if (lowIndex == fromIndex)
        {
            // Moving to different layer, which is moving diagonally up the stairs
            m_NavGraph->AddEdge(fromIndex, toIndex, Sqrt2);
            return;
        }
    }

    // Check if stairs are right below
    if (fromY - 1 < 0)
        return;
    toIndex = GetIndexFromPosition(toX, fromY - 1, toZ);
    toDef   = GetTileDefinition(m_Cells[toIndex]);
    if (toDef && toDef->isWalkable)
    {
        toDef->GetSlopeNeighboringTiles(toX, fromY - 1, toZ, low, high);
        if (high != Vector3Zeros)
        {
            unsigned highIndex = GetIndexFromPosition(
                static_cast<int>(high.x), static_cast<int>(high.y), static_cast<int>(high.z));
            if (highIndex == fromIndex)
            {
                // Moving to different layer, which is moving diagonally up the stairs
                m_NavGraph->AddEdge(fromIndex, toIndex, Sqrt2);
                return;
            }
        }
    }
}

bool Tilemap::AddEdgeFromSlopeToGraph(const TileDefinition* fromDef, unsigned fromIndex)
{
    if (fromDef->slope != '\0') // currently at slope
    {
        int x, y, z;
        GetPositionFromIndex(fromIndex, x, y, z);
        Vector3 low, high;
        fromDef->GetSlopeNeighboringTiles(x, y, z, low, high);

        unsigned lowIndex = GetIndexFromPosition(
            static_cast<int>(low.x), static_cast<int>(low.y), static_cast<int>(low.z));
        unsigned highIndex = GetIndexFromPosition(
            static_cast<int>(high.x), static_cast<int>(high.y), static_cast<int>(high.z));

        // Check if neighboring is connected to a walkable tile
        const TileDefinition* lowDef  = GetTileDefinition(m_Cells[lowIndex]);
        const TileDefinition* highDef = GetTileDefinition(m_Cells[highIndex]);
        if (lowDef && lowDef->isWalkable)
            m_NavGraph->AddEdge(fromIndex, lowIndex, Sqrt2);
        if (highDef && highDef->isWalkable)
            m_NavGraph->AddEdge(fromIndex, highIndex, Sqrt2);
        return true;
    }
    return false;
}

} // namespace eng
