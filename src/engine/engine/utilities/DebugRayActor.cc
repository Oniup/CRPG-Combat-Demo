#include "engine/utilities/DebugRayActor.h"
#include "engine/graphics/RenderQueue.h"
#include "engine/world/World.h"
#include "engine/world/actors/Actor.h"

#include "raylib.h"

namespace eng {

DebugRayActor::DebugRayActor(Vector3 position)
    : Actor(position),
      m_Timer(1.0f)
{
}

void DebugRayActor::OnInitialize(eng::ResourceManager& resources, Vector3 direction, float distance,
                                 Color color)
{
    m_Direction = Vector3Normalize(direction);
    m_Distance  = distance;
    m_Color     = color;
}

void DebugRayActor::OnBeginPlay(ResourceManager& resources)
{
    m_Timer.Start();
}

void DebugRayActor::OnUpdate(float deltaTime)
{
    if (m_Timer.IsComplete())
    {
        fmt::print("Hello...");
        GetWorld()->DestroyActor(this);
    }
}

void DebugRayActor::SubmitDrawCommands(RenderQueue& renderQueue)
{
    Vector3 endPos = position + m_Direction * m_Distance;
    renderQueue.Submit(DrawCommand{
        .shapeFn =
            [endPos](DrawCommand& cmd)
        {
            DrawLine3D(cmd.position, endPos, cmd.color);
        },
        .position = position,
        .color    = m_Color,
    });
}

} // namespace eng
