#include "main/MoveWeightedTilemap.h"

#include "engine/ai/InfluenceMap.h"
#include "engine/graphics/Model.h"
#include "engine/graphics/Texture.h"
#include "engine/world/LevelData.h"
#include "engine/world/actors/Tilemap.h"
#include "gameplay/Action.h"
#include "gameplay/Inventory.h"

namespace game {

void MoveWeightedTilemap::SetupRequiredResources(eng::ResourceManager& resources)
{
    auto* texture = resources.Load<eng::Texture>(
        "Tile Outline Texture", resources.GetAssetPath("textures/TileOutline.png"));
    auto* model = resources.Load<eng::Model>("Tile Model", eng::Model::CreateCube());
    model->AssignTexture(texture, MATERIAL_MAP_ALBEDO);
}

MoveWeightedTilemap::MoveWeightedTilemap(const Vector3& position)
    : eng::Tilemap(position)
{
}

void MoveWeightedTilemap::LoadFromLevelData(const eng::LevelData& levelData,
                                            eng::ResourceManager& resources)
{
    eng::Tilemap::LoadFromLevelData(levelData, resources);
    SetModel(resources.Get<eng::Model>("Tile Model"));

    resources.Load<eng::InfluenceMap>(
        Item::InfluenceMapName_PoisonPuddle, GetWidth(), GetHeight(), GetLength());

    auto* moveWeightCost = resources.Load<eng::InfluenceMap>(
        InfluenceMapName_MoveCostWeight, GetWidth(), GetHeight(), GetLength());

    IterateCells(
        [&](const eng::TileIterCellContext& cell)
        {
            const eng::LevelTypeDef& levelTileDef = levelData.tileDefinitions.at(cell.id);
            int weight                            = levelTileDef.GetField<int>("mCost", 1);
            moveWeightCost->SetSingleCellInfluence(cell.index, weight);
        });
}

} // namespace game
