#pragma once

#include "engine/ai/InfluenceMap.h"
#include <limits>
#include <tuple>
#include <unordered_map>

namespace eng {

struct LevelTypeDef;

}

namespace game {

struct Item
{
    static constexpr std::string_view HealthPotionName = "Health\nPotion";
    static constexpr std::string_view PoisonPotionName = "Poison\nPotion";
    static constexpr std::string_view SlimeBottleName  = "Slime\nBottle";
    static constexpr std::string_view FoodName         = "Food";
    static constexpr std::string_view ArrowName        = "Arrow";

    static constexpr std::string_view HealthPotionID = "HP";
    static constexpr std::string_view PoisonPotionID = "PO";
    static constexpr std::string_view SlimeBottleID  = "SL";
    static constexpr std::string_view FoodID         = "FO";
    static constexpr std::string_view ArrowID        = "AR";

    static constexpr std::string_view InfluenceMapName_PoisonPuddle = "Poison Puddle Influence Map";
    static constexpr std::string_view InfluenceMapName_SlimePuddle  = "Slime Puddle Influence Map";

    static const std::tuple<std::string_view, std::string_view, Item> AllTypes[];
    static const unsigned AllTypesSize;

    std::string_view name                 = nullptr;
    int addHealth                         = 0;     // Can subtract to do damage
    unsigned duration                     = 1;     // Measured in turns, will tick health every turn
    bool isConsumable                     = false; // Allows character to eat/drink item
    // Throwing
    bool isThrowable                      = false; // Allows character to throw item
    unsigned throwHitTileRadius           = 0;     // Radius from center hit point
    eng::InfluenceDropOffType dropOffType = eng::InfluenceDropOffType::Linear;
    // Movement
    float movementSpeedModifier           = 0.0f; // Increase/Decrease visualization movement
    int addMovementCost                   = 0;    // Modifies the cost of movement

    bool IsValid() const;

    // Defensive
    static Item CreateHealthPotionDef();
    static Item CreateFoodDef();
    // Offensive
    static Item CreatePoisonPotionDef();
    static Item CreateSlimeBottleDef();
    static Item CreateArrowDef();
};

class Inventory
{
public:
    static constexpr unsigned DoesNotContainItem = std::numeric_limits<unsigned>::max();
    using UnorderedMap = std::unordered_map<std::string_view, std::tuple<Item, unsigned>>;

    Inventory() = default;

    const UnorderedMap& GetEntries() const { return m_Items; }

    unsigned GetItemCount(const std::string_view& name) const;
    Item GetItem(const std::string_view& name) const;
    bool Contains(const std::string_view& name) const;

    void AddItemType(Item item);
    void AddItem(const std::string_view& itemName);
    void UseItem(const std::string_view& itemName);

    void AddItemsFromFields(const eng::LevelTypeDef* def);
    void DebugPrint() const;

private:
    UnorderedMap m_Items;
};

} // namespace game
