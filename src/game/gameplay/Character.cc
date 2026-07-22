#include "gameplay/Character.h"

#include "engine/ai/InfluenceMap.h"
#include "engine/ai/SparseGraph.h"
#include "engine/core/ResourceManager.h"
#include "engine/graphics/Model.h"
#include "engine/graphics/RenderQueue.h"
#include "engine/graphics/Texture.h"
#include "engine/utilities/Color.h"
#include "engine/utilities/DebugRayActor.h"
#include "engine/world/LevelData.h"
#include "engine/world/World.h"
#include "engine/world/actors/NavigationActor.h"
#include "gameplay/Action.h"
#include "gameplay/TurnManager.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <memory>

#define DEBUG_SHOOT_RADIUS
// #define DEBUG_LOS_WHEN_CALLED

namespace game {

void Character::SetupRequiredResources(eng::ResourceManager& resources)
{
    auto* teamIDTexture = resources.Load<eng::Texture>(
        "Team Identifier Base Texture", resources.GetAssetPath("textures/TeamIdentifier.png"));

    auto* teamIDMesh =
        resources.Load<eng::Model>("Team Identifier Base", eng::Model::CreatePlane());
    teamIDMesh->AssignTexture(teamIDTexture, MATERIAL_MAP_ALBEDO);
}

Character::Character(const Vector3& position)
    : eng::NavigationActor(position, 0.05f),
      m_CollisionBoundsSize(Vector3Ones * 0.5f),
      m_CollisionBoundsOffset(Vector3{.y = 0.25f}),
      m_Mass(0.4f),
      m_MaxSpeed(2.0f)
{
}

void Character::OnInitialize(eng::ResourceManager& resources, const eng::SpawnInfo& spawnInfo)
{
    // Identifiers
    m_TypeName = spawnInfo.definition->GetField<std::string>("ty");
    m_TeamID   = spawnInfo.definition->GetField<int>("tm");

    SetTag(std::string(TeamTags[m_TeamID]));

    // Visual
    m_Tint           = eng::ConvertHexToColor(spawnInfo.definition->GetField<std::string>("tint"));
    m_Height         = spawnInfo.definition->GetField<float>("height", 0.5f);
    m_VerticalOffset = m_Height * 0.25f;

    // Models
    m_TeamIdentifierBase = resources.Get<eng::Model>("Team Identifier Base");
    m_Body               = resources.Get<eng::Model>("Cylinder");
    DASSERT(m_TeamIdentifierBase, "Failed to load static model on startup");

    // Attributes
    m_MaxAP     = spawnInfo.definition->GetField<int>("maxAP", 10);
    m_MaxHealth = spawnInfo.definition->GetField<int>("maxHP", 20);
    m_Dexterity = spawnInfo.definition->GetField<float>("dex", 5.0f);

    // Current Stats
    m_Health        = m_MaxHealth;
    m_AvailableAP   = m_MaxAP;
    m_MaxShootRange = 20;

    // Required resources
    auto* graph         = resources.Get<eng::SparseGraph>("Navigation Graph");
    m_TileCostInfluence = resources.Get<eng::InfluenceMap>(InfluenceMapName_MoveCostWeight);

    InitializeNavigation( // Navigation and control
        graph,
        resources.GetList<eng::InfluenceMap>({
            eng::InfluenceMapName_HighGround,
            eng::InfluenceMapName_TileWeight,
        }));

    // Setup actions that every character contains
    m_Actions.emplace_back(std::make_unique<MoveToAction>(m_TileCostInfluence, this)); // Must front
    m_Actions.emplace_back(std::make_unique<ConsumeItem>(this));
    m_Actions.emplace_back(std::make_unique<ThrowItemAction>(this));

    // Specific character type setup
    if (m_TypeName == "Archer")
        OnInitializeArcherType(resources, spawnInfo);
    else if (m_TypeName == "Warrior")
        OnInitializeWarriorType(resources, spawnInfo);
    else
        FATAL("Invalid character type {}", m_TypeName);

    m_Inventory.AddItemsFromFields(spawnInfo.definition);
}

void Character::OnDestroy()
{
    auto* turnManager = GetWorld()->Find<TurnManager>();
    turnManager->RemoveCharacterFromTurnOrder(this);
}

void Character::RefreshAbilityPoints()
{
    m_AvailableAP  = m_MaxAP;
    m_SelectedItem = "";
}

bool Character::SpendAP(unsigned cost)
{
    if (cost <= m_AvailableAP)
    {
        m_AvailableAP -= cost;
        return true;
    }
    return false;
}

bool Character::HasEnoughAP(unsigned cost) const
{
    return cost <= m_AvailableAP;
}

void Character::ApplyHealth(int health)
{
    int oldHealth = m_Health;
    m_Health += health;
    if (m_Health > m_MaxHealth)
        m_Health = m_MaxHealth;
}

void Character::SetSelectedItem(const std::string_view& item)
{
    if (m_Inventory.Contains(item))
        m_SelectedItem = item;
}

bool Character::SetDestination(unsigned index, unsigned maxSteps)
{
    if (m_AvailableAP > 0)
    {
        if (eng::NavigationActor::SetDestination(index, maxSteps))
        {
            // MoveToAction will always be first in the list
            auto* action              = static_cast<MoveToAction*>(m_Actions.front().get());
            unsigned maxPossibleSteps = action->GetRouteMaxPossibleSteps(m_AvailableAP);

            PrunePathToMaxSteps(maxPossibleSteps);
            return true;
        }
    }
    return false;
}

bool Character::IsTargetInLOS(Actor* target, bool renderWhenHit)
{
    Vector3 startPos  = position + m_CollisionBoundsOffset * 0.5f;
    Vector3 direction = target->position - position;

    eng::RayHitInfo info = GetWorld()->GetRayHit(this, 0, startPos, direction);

    if ((renderWhenHit && info.hitActor) || GetWorld()->ShouldRenderDebugInfo())
    {
        float distance = 1000.0f;
        if (info.hitActor)
            distance = Vector3Distance(startPos, info.rayCollision.point);

        GetWorld()->Instantiate<eng::DebugRayActor>(startPos, direction, distance, m_Tint);
    }

    return info.hitActor && info.hitActor == target;
}

void Character::SetCurrentAction(unsigned index)
{
    DASSERT(index < m_Actions.size(), "Index out of range {} !< {}", index, m_Actions.size());
    m_CurrentAction = m_Actions[index].get();
}

void Character::OnBeginPlay(eng::ResourceManager& resources)
{
    for (std::unique_ptr<Action>& action : m_Actions)
        action->OnBeginPlay(resources);
}

void Character::OnUpdate(float deltaTime)
{
    if (m_Health <= 0)
    {
        GetWorld()->DestroyActor(this);
        return;
    }

    FollowPath();
    ApplyForces(deltaTime);
}

void Character::SubmitDrawCommands(eng::RenderQueue& renderQueue)
{
    Vector3 basePosition = position + Vector3UnitY * m_VerticalOffset;
    Vector3 topPosition  = basePosition + Vector3UnitY * m_Height;

    renderQueue.Submit( // Capsule representing the body of the character
        eng::DrawCommand{
            .model    = m_Body,
            .position = position,
            .scale    = Vector3(0.5f, m_Height, 0.5f),
            .color    = m_Tint,
        });
    renderQueue.Submit( // Circle base to identify which team the character is on
        eng::DrawCommand{
            .model    = m_TeamIdentifierBase,
            .position = position + Vector3UnitY * 0.05f,
            .color    = TeamTints[m_TeamID],
        },
        true);

    // Planned follow path line
    RenderPath(renderQueue, TeamTints[m_TeamID]);

    if (GetWorld()->ShouldRenderDebugInfo())
    {
        // Render debug collider
        Vector3 boundsSize = m_CollisionBoundsSize;
        renderQueue.Submit(eng::DrawCommand{
            .shapeFn =
                [boundsSize](eng::DrawCommand& cmd)
            {
                BoundingBox bounds = {
                    .min = cmd.position - boundsSize * 0.5f,
                    .max = cmd.position + boundsSize * 0.5f,
                };
                DrawBoundingBox(bounds, cmd.color);
            },
            .position = position + m_CollisionBoundsOffset,
            .color    = GREEN,
        });

#ifdef DEBUG_SHOOT_RADIUS
        // Render max shooting range
        if (m_TypeName == "Archer")
        {
            TurnManager* turnManager = GetWorld()->Find<TurnManager>();
            if (turnManager->IsCharactersTurn(this))
            {
                float maxShootingRange = m_MaxShootRange;
                renderQueue.Submit(eng::DrawCommand{
                    .shapeFn =
                        [maxShootingRange](eng::DrawCommand& cmd)
                    {
                        DrawSphereWires(cmd.position, std::sqrt(maxShootingRange), 8, 8, cmd.color);
                    },
                    .position = position,
                    .color    = m_Tint,
                });
            }
        }
#endif
    }
}

RayCollision Character::CheckRayCollision(const Ray& ray) const
{
    Vector3 boundsPosition = position + m_CollisionBoundsOffset;
    BoundingBox bounds     = {
        .min = boundsPosition - m_CollisionBoundsSize * 0.5f,
        .max = boundsPosition + m_CollisionBoundsSize * 0.5f,
    };

    RayCollision rayCollision = GetRayCollisionBox(ray, bounds);
    return rayCollision;
}

void Character::FollowPath()
{
    Vector3 nodePosition = GetPathNodePosition();
    if (nodePosition == eng::SparseGraph::Node::Invalid)
    {
        m_Velocity = eng::SparseGraph::Node::Invalid;
        return;
    }

    if (ReachedEndOfPath())
    {
        ApplyArriveForce(nodePosition);
        return;
    }

    if (ReachedPathNode())
        IncrementPathNode();
    ApplySeekForce(nodePosition);
}

void Character::ApplyArriveForce(Vector3 target)
{
    static constexpr float DecelerationRate = 1.5f;

    Vector3 toTarget = target - position;
    float distance   = Vector3LengthSqr(toTarget);
    if (distance > 0)
    {
        distance                = std::sqrt(distance);
        float speed             = std::min(distance / DecelerationRate, m_MaxSpeed);
        Vector3 desiredVelocity = toTarget * (speed / distance);
        m_Force                 = desiredVelocity - m_Velocity;
    }
    else
        m_Force = Vector3Zeros;
}

void Character::ApplySeekForce(Vector3 target)
{
    Vector3 moveDirection   = Vector3Normalize(target - position);
    Vector3 desiredVelocity = moveDirection * m_MaxSpeed;
    m_Force                 = desiredVelocity - m_Velocity;
}

void Character::ApplyForces(float deltaTime)
{
    if (m_Velocity == eng::SparseGraph::Node::Invalid)
    {
        m_Velocity = Vector3Zeros;
        return;
    }

    m_Velocity += (m_Force / m_Mass) * deltaTime;

    // Truncate velocity to max speed
    float CurrSpeedSqr = Vector3LengthSqr(m_Velocity);
    if (CurrSpeedSqr > m_MaxSpeed * m_MaxSpeed)
    {
        m_Velocity /= std::sqrt(CurrSpeedSqr);
        m_Velocity *= m_MaxSpeed;
    }

    position += m_Velocity * deltaTime;
}

void Character::OnInitializeArcherType(eng::ResourceManager& resources, const eng::SpawnInfo& info)
{
    // Setup required actions
    m_Actions.emplace_back(std::make_unique<ShootArrowAction>(this));
    m_Actions.emplace_back(std::make_unique<MeleeAction>(true, -3, this));
}

void Character::OnInitializeWarriorType(eng::ResourceManager& resources, const eng::SpawnInfo& info)
{
    // Setup required actions
    m_Actions.emplace_back(std::make_unique<MeleeAction>(false, -6, this));
}

} // namespace game
