#include "environment/Barrel.h"

#include "engine/core/ResourceManager.h"
#include "engine/graphics/RenderQueue.h"
#include "raylib.h"
#include "raymath.h"
#include <numbers>
#include <random>

namespace game {

void Barrel::SetupRequiredResources(eng::ResourceManager& resources)
{
    resources.Load<eng::Model>("Barrel Model", resources.GetAssetPath("models/Barrel.glb"));
}

Barrel::Barrel(const Vector3& position)
    : eng::Actor(position, "Barrel")
{
}

void Barrel::OnInitialize(eng::ResourceManager& resources, const eng::SpawnInfo& info)
{
    m_Model = resources.Get<eng::Model>("Barrel Model");

    // Random rotation
    int max = 6;
    std::random_device device;
    std::mt19937 engine(device());
    std::uniform_int_distribution dist(0, max);

    m_Rotation = (2 * std::numbers::pi / max) * dist(engine);
}

void Barrel::SubmitDrawCommands(eng::RenderQueue& renderQueue)
{
    eng::DrawCommand cmd{
        .model    = m_Model,
        .position = position + Vector3UnitY * 0.25f,
        .rotation = Vector3(0.0f, m_Rotation, 0.0f),
        .scale    = Vector3Ones * 0.3f,
        .color    = BROWN,
    };
    renderQueue.Submit(cmd);
}

} // namespace game
