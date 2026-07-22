#pragma once

#include "engine/world/actors/Actor.h"
#include "raylib.h"
#include <numbers>

namespace game {

class CameraController : public eng::Actor
{
    ENG_ACTOR(CameraController)

public:
    CameraController(Vector3 position);
    void OnInitialize(eng::ResourceManager& resources, int projection, float distanceFromTarget,
                      float fov = 45.0f);
    void OnDestroy() override;

    void OnBeginPlay(eng::ResourceManager& resources) override;
    void OnUpdate(float deltaTime) override;

private:
    void MoveController(float deltaTime);
    void RotateController(float deltaTime);
    void UpdateCamera();

    Camera3D m_Camera;
    Vector3 m_PositionOffset;
    float m_DistanceFromTarget;
    float m_Angle = std::numbers::pi_v<float> / 4.0f; // 45 degree angle in radians
    float m_LastAngle;

    float m_RotateSensitivity = 4.0f;
    float m_MoveSpeed         = 10.0f;

    float m_MouseMovementPadding = 0.1f;
    Vector2 m_LastMousePosition;
};

} // namespace game
