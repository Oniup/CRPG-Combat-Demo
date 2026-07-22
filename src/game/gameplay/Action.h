#pragma once

#include "engine/ai/InfluenceMap.h"
#include "engine/core/ResourceManager.h"
#include "engine/core/Timer.h"
#include <limits>
#include <string>
#include <string_view>

namespace eng {

class World;

}

namespace game {

class Character;
class TurnManager;

static constexpr std::string_view InfluenceMapName_MoveCostWeight = "Move Cost Weight Influence";

class Action
{
public:
    static constexpr unsigned CannotPerformCost  = std::numeric_limits<unsigned>::max();
    static constexpr double DefaultDelayDuration = 1.0;

    virtual ~Action();

    const std::string& GetName() const { return m_Name; }
    const Character* GetOwner() const { return m_Owner; }

    virtual void OnBeginPlay(eng::ResourceManager& resources) {}

    // Defines the Ability Point cost
    virtual unsigned GetCost() = 0;

    // Controls the duration of the action executing
    virtual bool Executing();

    // Must include `Action::OnExecute()` within overridden implementation as it makes the character
    // spend the AP to execute this action.
    virtual bool OnExecute();

    Character* CharacterExistsAt(unsigned x, unsigned y, unsigned z) const;

protected:
    Action(const std::string_view& name, Character* owner);

    Character* GetOwner() { return m_Owner; }
    eng::World* GetWorld();
    const eng::World* GetWorld() const;

    // Checks if the tile at x, y, z is within the given rangeRadiusSq. NOTE this does not call sqrt
    // therefore it will give an approximate and 2.4f is roughly 1 tile next to current (including
    // diagonals). It does not include high ground as that value is greater than 3.
    bool TargetInRange(unsigned x, unsigned y, unsigned z, float rangeRadiusSq = 2.4f) const;

    // Specify delay duration between performing actions.
    void SetDelayDuration(double duration);
    // Starts delay duration between performing actions.
    void StartDelayDuration();

private:
    std::string m_Name;
    Character* m_Owner;      // Character Actor that owns this Action within their action list
    eng::Timer m_DelayTimer; // Delay BTW actions, prevents AI from doing > 1 action instantly
};

class MoveToAction : public Action
{
public:
    MoveToAction(const eng::InfluenceMap* tileMoveCost, Character* owner);

    unsigned GetCost() override;
    bool Executing() override;
    bool OnExecute() override;

    unsigned GetRouteMaxPossibleSteps(unsigned currentAP);

private:
    const eng::InfluenceMap* m_TileCostInfluence;
};

class MeleeAction : public Action
{
public:
    MeleeAction(bool isArcher, int damage, Character* owner);

    int GetDamageAmount() const { return m_Damage; }

    void OnBeginPlay(eng::ResourceManager& resources) override;
    unsigned GetCost() override;
    bool OnExecute() override;

private:
    bool m_IsArcher;
    int m_Damage;
    TurnManager* m_TurnManager;
};

class ShootArrowAction : public Action
{
public:
    ShootArrowAction(Character* owner);

    void OnBeginPlay(eng::ResourceManager& resources) override;
    unsigned GetCost() override;
    bool OnExecute() override;

private:
    int m_Damage = -5;
    TurnManager* m_TurnManager;
};

class ThrowItemAction : public Action
{
public:
    ThrowItemAction(Character* owner);

    void OnBeginPlay(eng::ResourceManager& resources) override;
    unsigned GetCost() override;
    bool OnExecute() override;

private:
    TurnManager* m_TurnManager;
};

class ConsumeItem : public Action
{
public:
    ConsumeItem(Character* owner);

    unsigned GetCost() override;
    bool OnExecute() override;
};

} // namespace game
