#pragma once

#include "engine/core/ResourceManager.h"
#include "engine/core/Timer.h"
#include "engine/world/actors/Actor.h"

namespace eng {

class DebugRayActor : public Actor
{
    ENG_ACTOR(DebugRayActor)

public:
    DebugRayActor(Vector3 position);
    void OnInitialize(eng::ResourceManager& resources, Vector3 direction, float distance,
                      Color color);

    void OnBeginPlay(ResourceManager& resources) override;
    void OnUpdate(float deltaTime) override;
    void SubmitDrawCommands(RenderQueue& renderQueue) override;

private:
    Timer m_Timer;

    Vector3 m_Direction;
    float m_Distance;
    Color m_Color;
};

} // namespace eng
