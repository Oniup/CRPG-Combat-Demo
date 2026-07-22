#include "gameplay/Action.h"

#include "engine/core/ResourceManager.h"
#include "engine/world/World.h"
#include "gameplay/Character.h"
#include "gameplay/Inventory.h"
#include "gameplay/TurnManager.h"
#include "raymath.h"

namespace game {

Action::~Action()
{
    m_Owner = nullptr;
}

Action::Action(const std::string_view& name, Character* owner)
    : m_Name(name),
      m_Owner(owner),
      m_DelayTimer(DefaultDelayDuration)
{
}

bool Action::Executing()
{
    return !m_DelayTimer.IsComplete();
}

eng::World* Action::GetWorld()
{
    return m_Owner->GetWorld();
}

const eng::World* Action::GetWorld() const
{
    return m_Owner->GetWorld();
}

bool Action::OnExecute()
{
    bool spent = GetOwner()->SpendAP(GetCost());
    if (spent)
        StartDelayDuration();
    return spent;
}

bool Action::TargetInRange(unsigned x, unsigned y, unsigned z, float rangeRadiusSq) const
{
    Vector3 targetPos = {
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(z),
    };
    Vector3 direction = targetPos - GetOwner()->position;
    float length_sq   = Vector3LengthSqr(direction);
    return length_sq < rangeRadiusSq; // Tile next to or the same tile as the actor
}

Character* Action::CharacterExistsAt(unsigned x, unsigned y, unsigned z) const
{
    auto* tilemap                        = GetWorld()->GetTilemap();
    std::vector<const Character*> result = GetWorld()->FindAll<Character>();

    for (const Character* character : result)
    {
        if (character == GetOwner())
            continue;

        int cx, cy, cz;
        tilemap->ConvertVectorToPosition(character->position, cx, cy, cz);
        if (cx == x && cy == y && cz == z)
            return const_cast<Character*>(character);
        // ok, ok, I know this const_cast is bad, but I'm on a time limit... so shh...
    }
    return nullptr;
}

void Action::SetDelayDuration(double duration)
{
    m_DelayTimer.SetDuration(duration);
}

void Action::StartDelayDuration()
{
    m_DelayTimer.Start();
}

MoveToAction::MoveToAction(const eng::InfluenceMap* tileMoveCost, Character* owner)
    : Action("Move To", owner),
      m_TileCostInfluence(tileMoveCost)
{
    DASSERT(m_TileCostInfluence, "Must set tileMoveCost for MoveToAction to function");

    SetDelayDuration(0.0f); // This action doesn't require delay
}

unsigned MoveToAction::GetCost()
{
    const std::vector<unsigned>* fullRoute = GetOwner()->GetFullRoute();
    if (fullRoute)
    {
        float cost = 0;
        for (unsigned index : *fullRoute)
            m_TileCostInfluence->ApplyInfluence(cost, index);

        return static_cast<unsigned>(std::floor(cost));
    }
    return 0;
}

bool MoveToAction::OnExecute()
{
    if (Action::OnExecute())
    {
        GetOwner()->TraversePath();
        return true;
    }
    return false;
}

unsigned MoveToAction::GetRouteMaxPossibleSteps(unsigned currentAP)
{
    const std::vector<unsigned>* route = GetOwner()->GetFullRoute();

    float cost     = 0;
    unsigned steps = 0;
    for (unsigned index : *route)
    {
        if (static_cast<unsigned>(cost) >= currentAP)
            break;

        m_TileCostInfluence->ApplyInfluence(cost, index);
        steps++;
    }
    return steps;
}

bool MoveToAction::Executing()
{
    return !GetOwner()->ReachedEndOfPath();
}

MeleeAction::MeleeAction(bool isArcher, int damage, Character* owner)
    : Action("Melee", owner),
      m_IsArcher(isArcher),
      m_Damage(damage)
{
}

void MeleeAction::OnBeginPlay(eng::ResourceManager& resources)
{
    m_TurnManager = GetWorld()->Find<TurnManager>();
}

unsigned MeleeAction::GetCost()
{
    unsigned selectedX = m_TurnManager->GetSelectedX();
    unsigned selectedY = m_TurnManager->GetSelectedY();
    unsigned selectedZ = m_TurnManager->GetSelectedZ();
    if (!TargetInRange(selectedX, selectedY, selectedZ))
        return CannotPerformCost;

    Character* character = CharacterExistsAt(selectedX, selectedY, selectedZ);
    if (!character || character->GetTeamID() == GetOwner()->GetTeamID())
        return CannotPerformCost;

    if (m_IsArcher)
        return 3;
    return 4;
}

bool MeleeAction::OnExecute()
{
    unsigned selectedX = m_TurnManager->GetSelectedX();
    unsigned selectedY = m_TurnManager->GetSelectedY();
    unsigned selectedZ = m_TurnManager->GetSelectedZ();
    if (TargetInRange(selectedX, selectedY, selectedZ))
    {
        Character* character = CharacterExistsAt(selectedX, selectedY, selectedZ);
        if (character && character->GetTeamID() != GetOwner()->GetTeamID() && Action::OnExecute())
        {
            character->ApplyHealth(m_Damage);
            return true;
        }
    }

    return false;
}

ShootArrowAction::ShootArrowAction(Character* owner)
    : Action("Shoot\nArrow", owner)
{
}

void ShootArrowAction::OnBeginPlay(eng::ResourceManager& resources)
{
    m_TurnManager = GetWorld()->Find<TurnManager>();
}

unsigned ShootArrowAction::GetCost()
{
    if (!GetOwner()->GetInventory().Contains(Item::ArrowID))
        return CannotPerformCost;

    unsigned selectedX = m_TurnManager->GetSelectedX();
    unsigned selectedY = m_TurnManager->GetSelectedY();
    unsigned selectedZ = m_TurnManager->GetSelectedZ();
    if (!TargetInRange(selectedX, selectedY, selectedZ, GetOwner()->GetMaxShootRange()))
        return CannotPerformCost;

    Character* character = CharacterExistsAt(selectedX, selectedY, selectedZ);
    if (character && character->GetTeamID() != GetOwner()->GetTeamID())
        return 4;
    return CannotPerformCost;
}

bool ShootArrowAction::OnExecute()
{
    if (GetCost() == CannotPerformCost || !Action::OnExecute())
        return false;

    unsigned selectedX   = m_TurnManager->GetSelectedX();
    unsigned selectedY   = m_TurnManager->GetSelectedY();
    unsigned selectedZ   = m_TurnManager->GetSelectedZ();
    Character* character = CharacterExistsAt(selectedX, selectedY, selectedZ);

    character->ApplyHealth(m_Damage);
    GetOwner()->GetInventory().UseItem(Item::ArrowID);
    return true;
}

ThrowItemAction::ThrowItemAction(Character* owner)
    : Action("Throw\nItem", owner)
{
}

void ThrowItemAction::OnBeginPlay(eng::ResourceManager& resources)
{
    m_TurnManager = GetWorld()->Find<TurnManager>();
}

unsigned ThrowItemAction::GetCost()
{
    // Check if there is an item selected and it exists in the inventory
    const std::string_view& selected = GetOwner()->GetSelectedItem();
    Inventory& inventory             = GetOwner()->GetInventory();
    if (selected.empty() || !inventory.Contains(selected))
        return CannotPerformCost;

    // Verify the item can actually be thrown
    if (!inventory.GetItem(selected).isThrowable)
        return CannotPerformCost;

    // Grab target coordinates
    unsigned selectedX = m_TurnManager->GetSelectedX();
    unsigned selectedY = m_TurnManager->GetSelectedY();
    unsigned selectedZ = m_TurnManager->GetSelectedZ();

    // Verify the target is in range (using shoot range as a base)
    if (!TargetInRange(selectedX, selectedY, selectedZ, GetOwner()->GetMaxShootRange()))
        return CannotPerformCost;

    // Check if there is a character at the target tile
    Character* character = CharacterExistsAt(selectedX, selectedY, selectedZ);
    if (!character)
        return CannotPerformCost;

    return 3; // The AP cost for throwing an item
}

bool ThrowItemAction::OnExecute()
{
    // Ensure cost is valid and deduct AP via the base class
    if (GetCost() == CannotPerformCost || !Action::OnExecute())
        return false;

    unsigned selectedX   = m_TurnManager->GetSelectedX();
    unsigned selectedY   = m_TurnManager->GetSelectedY();
    unsigned selectedZ   = m_TurnManager->GetSelectedZ();
    Character* character = CharacterExistsAt(selectedX, selectedY, selectedZ);

    if (character)
    {
        const std::string_view& selected = GetOwner()->GetSelectedItem();
        Inventory& inventory             = GetOwner()->GetInventory();
        const Item& item                 = inventory.GetItem(selected);

        // Apply item's effect to the target character and remove it from thrower's inventory
        character->ApplyHealth(item.addHealth);
        inventory.UseItem(selected);

        return true;
    }

    return false;
}

ConsumeItem::ConsumeItem(Character* owner)
    : Action("Consume\nItem", owner)
{
}

unsigned ConsumeItem::GetCost()
{
    const std::string_view& selected = GetOwner()->GetSelectedItem();
    Inventory& inventory             = GetOwner()->GetInventory();
    if (selected.empty() || !inventory.Contains(selected))
        return CannotPerformCost;

    return inventory.GetItem(selected).isConsumable ? 2 : CannotPerformCost;
}

bool ConsumeItem::OnExecute()
{
    const std::string_view& selected = GetOwner()->GetSelectedItem();
    Inventory& inventory             = GetOwner()->GetInventory();
    if (selected.empty() || !inventory.Contains(selected))
        return false;

    const Item& item = inventory.GetItem(selected);
    if (item.isConsumable && Action::OnExecute())
    {
        GetOwner()->ApplyHealth(item.addHealth);
        GetOwner()->GetInventory().UseItem(selected);
        return true;
    }
    return false;
}

} // namespace game
