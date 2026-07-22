#include "gameplay/CameraController.h"

#include "engine/world/World.h"
#include "engine/world/actors/Actor.h"
#include "engine/world/actors/Tilemap.h"
#include "raylib.h"
#include "raymath.h"

namespace game {

CameraController::CameraController(Vector3 position)
    : eng::Actor(position, "Camera Controller")
{
}

void CameraController::OnInitialize(eng::ResourceManager& resources, int projection,
                                    float distanceFromTarget, float fov)
{
    m_Camera.up          = Vector3(0.0f, 1.0f, 0.0f);
    m_Camera.fovy        = fov;
    m_Camera.projection  = projection;
    m_DistanceFromTarget = distanceFromTarget;

    GetWorld()->SetMainCamera(&m_Camera);
}

void CameraController::OnDestroy()
{
    GetWorld()->SetMainCamera(nullptr);
}

void CameraController::OnBeginPlay(eng::ResourceManager& resources)
{
    // Get center of tilemap so camera can be looking directly in the middle
    eng::Tilemap* tilemap = GetWorld()->GetTilemap();
    position              = Vector3{
        .x = static_cast<float>(tilemap->GetWidth()) * 0.5f,
        .y = 0.0f,
        .z = static_cast<float>(tilemap->GetWidth()) * 0.5f,
    };
}

void CameraController::OnUpdate(float deltaTime)
{
    RotateController(deltaTime);
    MoveController(deltaTime);
    UpdateCamera();

    m_LastMousePosition = GetMousePosition();
}

void CameraController::MoveController(float deltaTime)
{
    Vector3 forward     = Vector3Normalize(position - (m_PositionOffset + position));
    forward.y           = 0.0f;
    const Vector3 right = Vector3CrossProduct(forward, Vector3UnitY);

    Vector3 moveDirection = Vector3Zeros;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        moveDirection -= right;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        moveDirection += right;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        moveDirection += forward;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        moveDirection -= forward;

    // Apply movement
    if (Vector3LengthSqr(moveDirection) != 0)
    {
        moveDirection = Vector3Normalize(moveDirection);
        moveDirection *= m_MoveSpeed * deltaTime;

        position += moveDirection;
    }
}

void CameraController::RotateController(float deltaTime)
{
    float angleChange = 0.0f;
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON))
    {
        float mouseX  = GetMouseX();
        float padding = 0.1f;
        if (mouseX < m_LastMousePosition.x - m_MouseMovementPadding ||
            mouseX > m_LastMousePosition.x + m_MouseMovementPadding)
        {
            float length = mouseX - m_LastMousePosition.x;
            angleChange += length;
        }
    }

    if (IsKeyDown(KEY_E))
        angleChange -= 1.0f * m_RotateSensitivity * 0.2f;
    if (IsKeyDown(KEY_Q))
        angleChange += 1.0f * m_RotateSensitivity * 0.2f;
    if (IsKeyDown(KEY_SPACE))
    {
        m_DistanceFromTarget += m_MoveSpeed * deltaTime;
        m_LastAngle++;
    }
    if (IsKeyDown(KEY_LEFT_CONTROL))
    {
        m_DistanceFromTarget -= m_MoveSpeed * deltaTime;
        m_LastAngle++;
    }

    float mouseScroll = GetMouseWheelMove();
    if (mouseScroll != 0.0f)
    {
        if (m_Camera.projection == CAMERA_ORTHOGRAPHIC)
            m_Camera.fovy -= mouseScroll * 1000.0f * deltaTime;
        else
            m_DistanceFromTarget -= mouseScroll * 1000.0f * deltaTime;

        m_LastAngle++;
    }

    if (angleChange != 0)
        m_Angle += angleChange * m_RotateSensitivity * deltaTime;
}

void CameraController::UpdateCamera()
{
    // Prevent cos and sin being called every frame
    if (m_LastAngle != m_Angle)
    {
        constexpr float pitch = std::numbers::pi_v<float> / 4.0f;
        Vector3 rotation{
            .x = std::cosf(pitch) * std::cosf(m_Angle),
            .y = std::sinf(pitch),
            .z = std::cosf(pitch) * std::sinf(m_Angle),
        };
        m_PositionOffset = rotation * m_DistanceFromTarget;
        m_LastAngle      = m_Angle;
    }

    m_Camera.target   = position;
    m_Camera.position = position + m_PositionOffset;
}

} // namespace game
