#include "gameplay/Inventory.h"

#include "engine/core/Error.h"
#include "engine/world/LevelData.h"
#include <tuple>

namespace game {

const std::tuple<std::string_view, std::string_view, Item> Item::AllTypes[] = {
    std::make_tuple(Item::HealthPotionID, Item::HealthPotionName, Item::CreateHealthPotionDef()),
    std::make_tuple(Item::PoisonPotionID, Item::PoisonPotionName, Item::CreatePoisonPotionDef()),
    std::make_tuple(Item::SlimeBottleID, Item::SlimeBottleName, Item::CreateSlimeBottleDef()),
    std::make_tuple(Item::FoodID, Item::FoodName, Item::CreateFoodDef()),
    std::make_tuple(Item::ArrowID, Item::ArrowName, Item::CreateArrowDef()),
};

const unsigned Item::AllTypesSize = sizeof(Item::AllTypes) / sizeof(Item::AllTypes[0]);

bool Item::IsValid() const
{
    return !name.empty();
}

Item Item::CreateHealthPotionDef()
{
    return Item{
        .name               = HealthPotionName,
        .addHealth          = 10,
        .isConsumable       = true,
        .isThrowable        = true,
        .throwHitTileRadius = 1,
    };
}

Item Item::CreateFoodDef()
{
    return Item{
        .name         = FoodName,
        .addHealth    = 5,
        .isConsumable = true,
    };
}

Item Item::CreatePoisonPotionDef()
{
    return Item{
        .name               = PoisonPotionName,
        .addHealth          = -5,
        .duration           = 3,
        .isThrowable        = true,
        .throwHitTileRadius = 3,
    };
}

Item Item::CreateSlimeBottleDef()
{
    return Item{
        .name               = SlimeBottleName,
        .duration           = 4,
        .isThrowable        = true,
        .throwHitTileRadius = 4,
    };
}

Item Item::CreateArrowDef()
{
    return Item{
        .name      = ArrowName,
        .addHealth = 20,
    };
}

unsigned Inventory::GetItemCount(const std::string_view& name) const
{
    if (!m_Items.contains(name))
        return DoesNotContainItem;
    return std::get<unsigned>(m_Items.at(name));
}

Item Inventory::GetItem(const std::string_view& name) const
{
    DASSERT(Contains(name),
            "Must contain {} item when calling this function. If unsure, use Contains beforehand",
            name);

    return get<Item>(m_Items.at(name));
}

bool Inventory::Contains(const std::string_view& name) const
{
    return m_Items.contains(name);
}

void Inventory::AddItemType(Item item)
{
    m_Items.emplace(item.name, std::make_tuple(item, 0u));
}

void Inventory::AddItem(const std::string_view& itemName)
{
    if (m_Items.contains(itemName))
    {
        std::get<unsigned>(m_Items.at(itemName))++;
    }
    ERROR("Inventory does not contain item with name {}", itemName);
}

void Inventory::UseItem(const std::string_view& itemName)
{
    if (m_Items.contains(itemName))
    {
        unsigned& count = std::get<unsigned>(m_Items.at(itemName));
        count           = count - 1;

        // Remove item from definition when fully depleated
        if (count == 0)
            m_Items.erase(itemName);
        return;
    }
    ERROR("Inventory does not contain item with name {}", itemName);
}

void Inventory::AddItemsFromFields(const eng::LevelTypeDef* def)
{
    DASSERT(def, "Must be defined");

    for (unsigned i = 0; i < Item::AllTypesSize; i++)
    {
        const auto& [itemName, name, itemDef] = Item::AllTypes[i];
        const int* result                     = def->TryGetField<int>(itemName);
        if (result && *result > 0)
            m_Items.emplace(itemName, std::make_tuple(itemDef, *result));
    }
}

void Inventory::DebugPrint() const
{
    for (const auto& [key, value] : m_Items)
    {
        const auto& [itemDef, count] = value;
        fmt::print("-\tItem Name: {}, Count: {}\n", itemDef.name, count);
    }
}

} // namespace game
