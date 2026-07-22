#pragma once

#include "engine/core/ResourceManager.h"
#include "engine/world/actors/Tilemap.h"

namespace game {

class MoveWeightedTilemap : public eng::Tilemap
{
    ENG_ACTOR(MoveWeightedTilemap)

public:
    static void SetupRequiredResources(eng::ResourceManager& resources);

    MoveWeightedTilemap(const Vector3& position);

    void LoadFromLevelData(const eng::LevelData& levelData,
                           eng::ResourceManager& resources) override;
};

} // namespace game
