#pragma once

#include "engine/ai/AStarPath.h"
#include "engine/ai/InfluenceMap.h"
#include "engine/core/ResourceManager.h"
#include "engine/graphics/RenderQueue.h"
#include "engine/world/actors/Actor.h"
#include "engine/world/actors/NavigationActor.h"
#include "gameplay/Action.h"
#include "gameplay/Inventory.h"
#include "raylib.h"
#include <array>
#include <memory>

namespace eng {

class Model;

}

namespace game {

class Character;
class Action;

class Character : public eng::NavigationActor
{
    ENG_ACTOR(Character)

public:
    static constexpr int TeamTintsAlpha             = 150;
    static constexpr std::array<Color, 3> TeamTints = {
        Color{GREEN.r,  GREEN.g,  GREEN.b,  TeamTintsAlpha},
        Color{RED.r,    RED.g,    RED.b,    TeamTintsAlpha},
        Color{ORANGE.r, ORANGE.g, ORANGE.b, TeamTintsAlpha},
    };
    static constexpr std::array<std::string_view, 2> TeamTags = {
        "Player",
        "Agent",
    };

    static void SetupRequiredResources(eng::ResourceManager& resources);

    // Lifetime
    Character(const Vector3& position);
    void OnInitialize(eng::ResourceManager& resources, const eng::SpawnInfo& spawnInfo) override;
    void OnDestroy() override;

    // Runtime loop
    void OnBeginPlay(eng::ResourceManager& resources) override;
    void OnUpdate(float deltaTime) override;
    void SubmitDrawCommands(eng::RenderQueue& renderQueue) override;

    RayCollision CheckRayCollision(const Ray& ray) const override;

    // Ability Points
    unsigned GetMaxAP() const { return m_MaxAP; }
    unsigned GetAvailableAP() const { return m_AvailableAP; }
    void RefreshAbilityPoints();
    bool SpendAP(unsigned cost);
    bool HasEnoughAP(unsigned cost) const;

    // Health
    int GetMaxHealth() const { return m_MaxHealth; }
    int GetHealth() const { return m_Health; }
    bool IsDead() const { return m_Health < 0; }
    void ApplyHealth(int health);

    // Inventory
    Inventory& GetInventory() { return m_Inventory; }
    const Inventory& GetInventory() const { return m_Inventory; }
    const std::string_view& GetSelectedItem() const { return m_SelectedItem; }
    void SetSelectedItem(const std::string_view& item = "");

    // Navigation
    bool SetDestination(unsigned index, unsigned maxSteps = eng::AStarPath::InfiniteSteps) override;
    bool IsTargetInLOS(Actor* target, bool renderWhenHit = false);

    // Actions
    const std::vector<std::unique_ptr<Action>>& GetActions() const { return m_Actions; }
    Action* GetCurrentAction() { return m_CurrentAction; }
    void SetCurrentAction(unsigned index);

    // Other Getters
    virtual bool IsAI() const { return false; }
    unsigned GetDexerity() const { return m_Dexterity; }
    const std::string& GetTypeName() const { return m_TypeName; }
    Color GetTint() const { return m_Tint; }
    unsigned GetTeamID() const { return m_TeamID; }
    float GetMaxShootRange() const { return m_MaxShootRange; }

private:
    void FollowPath();

    // Physics
    void ApplyArriveForce(Vector3 target);
    void ApplySeekForce(Vector3 target);
    void ApplyForces(float deltaTime);

    void OnInitializeArcherType(eng::ResourceManager& resources, const eng::SpawnInfo& info);
    void OnInitializeWarriorType(eng::ResourceManager& resources, const eng::SpawnInfo& info);

    // Graphics
    Color m_Tint;
    eng::Model* m_TeamIdentifierBase;
    eng::Model* m_Body;
    float m_VerticalOffset;
    float m_Height;

    // Physics
    Vector3 m_CollisionBoundsSize;
    Vector3 m_CollisionBoundsOffset;
    float m_Mass;
    float m_MaxSpeed;
    Vector3 m_Velocity;
    Vector3 m_Force;

    // Actions and Items
    std::vector<std::unique_ptr<Action>> m_Actions;
    Action* m_CurrentAction = nullptr;
    Inventory m_Inventory;
    bool m_ExecutingAction;

    // Navigation
    eng::InfluenceMap* m_TileCostInfluence;

    // Stats
    std::string m_TypeName;
    unsigned m_MaxAP;
    unsigned m_AvailableAP;
    unsigned m_Dexterity;
    unsigned m_TeamID;
    std::string_view m_SelectedItem;
    int m_Health;
    int m_MaxHealth;
    float m_MaxShootRange;
};

} // namespace game
