#pragma once

#include "engine/graphics/Model.h"
#include "engine/graphics/RenderQueue.h"
#include "engine/world/actors/Actor.h"

namespace game {

class Barrel : public eng::Actor
{
    ENG_ACTOR(Barrel)

public:
    static void SetupRequiredResources(eng::ResourceManager& resources);

    Barrel(const Vector3& position);
    void OnInitialize(eng::ResourceManager& resources, const eng::SpawnInfo& info) override;

    void SubmitDrawCommands(eng::RenderQueue& renderQueue) override;

private:
    eng::Model* m_Model;
    float m_Rotation;
};

} // namespace game
