#pragma once

#include "engine/core/ResourceManager.h"
#include "engine/world/actors/Actor.h"
#include "raylib.h"
#include <functional>
#include <unordered_map>

#define ENG_TILEMAP_ITER_CELL_LAMBDA_PARAMS                                                        \
    unsigned x, unsigned y, unsigned z, unsigned index, unsigned cellId,                           \
        const eng::TileDefinition *def

namespace eng {

class LevelData;
class SparseGraph;
class Model;

struct TileDefinition
{
#ifndef NDEBUG
    static constexpr unsigned MaxNameBufferSize = 32;
    char nameBuffer[MaxNameBufferSize];
#endif
    Vector3 size;    // Size scaler
    Vector3 offset;  // Offset from center position
    Color tint;      // Color tint over texture
    bool isWalkable; // Whether Nav Graph should count as a walkable node
    int baseCost;    // Tile cost to influence path finding result
    char slope;      // n, e, s, w, or disable with \0

    Vector3 GetPositionOffset(int x, int y, int z) const;
    void GetSlopeNeighboringTiles(int currX, int currY, int currZ, Vector3& low,
                                  Vector3& high) const;
};

// Context for when using eng::Tilemap::IterateCells
struct TileIterCellContext
{
    int x;
    int y;
    int z;
    unsigned index;
    unsigned id;
    const TileDefinition* def;
};
// Lambda for when using eng::Tilemap::IterateCells
using Pfn_TilemapIterateCells = std::function<void(const TileIterCellContext& cell)>;

// Represents the box map visually. Defines Tile properties and positions within the
// world. World is constructed through layers which stack on top of each other.
class Tilemap : public Actor
{
    ENG_ACTOR(Tilemap)

public:
    // Loads map from level data.
    Tilemap(const Vector3& position, std::string&& tagId = "tilemap");
    virtual void LoadFromLevelData(const LevelData& levelData, ResourceManager& resources);

    unsigned GetWidth() const { return m_Width; }
    unsigned GetHeight() const { return m_Height; }
    unsigned GetLength() const { return m_Length; }
    float GetCellWidth() const { return m_CellWidth; }
    float GetCellHeight() const { return m_CellHeight; }

    unsigned GetCellTileID(unsigned index) const; // Cell Id used to query the TileDefinition
    const TileDefinition* GetTileDefinition(unsigned id) const; // Use GetCellTileID to get info
    const TileDefinition* GetTileDefinition(int x, int y, int z) const;
    // Converts x, y, z into an index that maps to the packed data
    int GetIndexFromPosition(int x, int y, int z) const;
    // Converts the index mapped to the packed data, into x, y, z position
    void GetPositionFromIndex(unsigned index, int& x, int& y, int& z) const;
    void ConvertVectorToPosition(Vector3 pos, int& x, int& y, int& z) const;
    bool IsValidIndex(unsigned index) const;

    // Submits the entire tilemap to the render queue, each tile having its own draw call.
    void SubmitDrawCommands(RenderQueue& renderQueue) override;

    // Shoot ray at tilemap and returns closest hit target
    RayCollision CheckRayCollision(const Ray& ray) const override;

    // Iterates through to each cell, skipping the air tiles
    void IterateCells(Pfn_TilemapIterateCells atCellFunc) const;

protected:
    void SetModel(Model* model);

private:
    // Populates _tile_defs using from `LevelData::tile_definitions`
    void LoadTileDefinitions(const LevelData& levelData);
    // Populates tile map cells with tile id which can query tile definitions when needed
    void LoadTilesFromLayer(const LevelData& levelData);

    // Setup Sparse Graph that maps to the Tilemap allowing Actors that implement A* to traverse the
    // tiles (eg. actors that inherit NavigationActor).
    void SetupNavGraph(ResourceManager& resources);
    // Helper function of SetupNavGraph -> Adds edge between to tiles of the tilemap, isDiagonal ==
    // true increases the directional cost from 1 to sqrt(2).
    void AddEdgeToNavGraph(unsigned fromIndex, int fromY, int toX, int toZ,
                           bool isDiagonal = false);
    // Implements slopes, allowing agents that implement A* to traverse from low ground to high
    // ground and vies versa.
    bool AddEdgeFromSlopeToGraph(const TileDefinition* fromDef, unsigned fromIndex);

    unsigned m_Width;
    unsigned m_Height;
    unsigned m_Length;
    float m_CellWidth;
    float m_CellHeight;
    unsigned m_NonAirCellCount = 0;
    SparseGraph* m_NavGraph;
    Model* m_TileModel;
    std::unordered_map<unsigned, TileDefinition> m_TileDefs;
    std::vector<unsigned> m_Cells;
};

} // namespace eng
