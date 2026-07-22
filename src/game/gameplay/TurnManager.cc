#include "gameplay/TurnManager.h"

#include "engine/core/Error.h"
#include "engine/core/ResourceManager.h"
#include "engine/graphics/RenderQueue.h"
#include "engine/world/World.h"
#include "engine/world/actors/Tilemap.h"
#include "fmt/base.h"
#include "gameplay/Action.h"
#include "gameplay/Character.h"
#include "raygui.h"
#include "raylib.h"
#include <algorithm>
#include <memory>

namespace game {

constexpr unsigned MaxTextBuffer = 1024;

bool TurnManager::TurnOrderEntry::operator>(const TurnOrderEntry& other) const
{
    DASSERT(other.character && character, "character field cannot be nullptr");
    return character->GetDexerity() > other.character->GetDexerity();
}

TurnManager::TurnManager(const Vector3& position)
    : eng::Actor(position, "Turn Manager")
{
}

void TurnManager::OnInitialize(eng::ResourceManager& resources, const eng::SpawnInfo& spawnInfo)
{
    m_SelectedTileModel = resources.Get<eng::Model>("Cube");
}

unsigned TurnManager::GetSelectedIndex() const
{
    return m_SelectedIndex;
}

unsigned TurnManager::GetSelectedX() const
{
    return m_SelectedX;
}

unsigned TurnManager::GetSelectedY() const
{
    return m_SelectedY;
}

unsigned TurnManager::GetSelectedZ() const
{
    return m_SelectedZ;
}

void TurnManager::RemoveCharacterFromTurnOrder(Character* remove)
{
    for (unsigned i = 0; i < m_TurnOrder.size(); i++)
    {
        if (m_TurnOrder[i].character == remove)
        {
            m_TurnOrder.erase(m_TurnOrder.begin() + i);
            if (m_SelectedCharacter >= i)
                m_SelectedCharacter = 0;
            if (m_CurrentTurn >= i)
                m_CurrentTurn--;
            return;
        }
    }
    ERROR("Failed to find character with address {} (Type: {}) within turn order",
          static_cast<const void*>(remove),
          remove->GetTypeName());
}

bool TurnManager::IsCharactersTurn(Character* character) const
{
    return m_TurnOrder[m_CurrentTurn].character == character;
}

void TurnManager::OnBeginPlay(eng::ResourceManager& resources)
{
    // Find all Characters within the scene
    std::vector<Character*> playerTeam = GetWorld()->FindAll<Character>();

    fmt::print("yes\n");

    m_TurnOrder.reserve(playerTeam.size());
    for (Character* character : playerTeam)
        m_TurnOrder.emplace_back(TurnOrderEntry{.character = character});

    // Sorts based on character's dexterity value
    std::sort(m_TurnOrder.begin(), m_TurnOrder.end(), std::greater<TurnOrderEntry>());
}

void TurnManager::OnUpdate(float deltaTime)
{
    Action* action = m_TurnOrder[m_CurrentTurn].character->GetCurrentAction();
    if (action && !action->Executing())
        m_ExecutingAction = false;

    // Selecting a tile
    if (!m_TurnOrder[m_CurrentTurn].character->IsAI())
    {
        if (m_ExecutingAction)
            return;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            // Hovering over character abilities panel, don't continue to selecting the world
            Vector2 mousePosition = GetMousePosition();
            if (CheckCollisionPointRec(mousePosition, m_CharAbilitiesRenderBounds))
                return;

            SelectCellInTilemap(mousePosition);
        }

        if (IsKeyPressed(KEY_ENTER))
            IncrementNextTurn();
    }
}

void TurnManager::SelectCellInTilemap(Vector2 mousePosition)
{
    m_SelectedTileDef = nullptr;

    // Select tile
    Ray ray                 = GetScreenToWorldRay(mousePosition, *GetWorld()->GetMainCamera());
    eng::RayHitInfo hitInfo = GetWorld()->GetRayAtMouse(this, eng::Actor::GetTagID("tilemap"));
    if (hitInfo.Hit())
    {
        eng::Tilemap* tilemap = static_cast<eng::Tilemap*>(hitInfo.hitActor);

        tilemap->ConvertVectorToPosition(
            hitInfo.rayCollision.point, m_SelectedX, m_SelectedY, m_SelectedZ);
        m_SelectedIndex = tilemap->GetIndexFromPosition(m_SelectedX, m_SelectedY, m_SelectedZ);
        if (!tilemap->IsValidIndex(m_SelectedIndex))
        {
            m_SelectedTileDef = nullptr;
            return;
        }

        unsigned tileId                    = tilemap->GetCellTileID(m_SelectedIndex);
        const eng::TileDefinition* tileDef = tilemap->GetTileDefinition(tileId);
        if (!tileDef)
            return;

        m_SelectedTileDef = tileDef;

        // Set agent to a new destination
        Character* character = m_TurnOrder[m_CurrentTurn].character;
        bool result          = character->SetDestination(m_SelectedIndex);
        if (!result)
        {
            ERROR("Failed to calculate path to {}, {}, {} : {}",
                  m_SelectedX,
                  m_SelectedY,
                  m_SelectedZ,
                  m_SelectedIndex);

            m_SelectedTileDef = nullptr;
            return;
        }
    }
}

void TurnManager::SubmitDrawCommands(eng::RenderQueue& renderQueue)
{
    if (m_SelectedTileDef)
    {
        float scaler =
            std::clamp(std::abs(std::sinf(GetTime()) * std::cosf(GetTime())), 0.0f, 1.0f);

        renderQueue.Submit(
            eng::DrawCommand{
                .model = m_SelectedTileModel,
                .position =
                    m_SelectedTileDef->GetPositionOffset(m_SelectedX, m_SelectedY, m_SelectedZ),
                .scale = m_SelectedTileDef->size * 1.01f,
                .color = Color(255, 255, 255, 200 * scaler),
            },
            true);
    }
}

void TurnManager::OnRenderUI()
{
    RenderTurnOrderUI();
    RenderCharacterActionUI();
}

void TurnManager::RenderTurnOrderUI()
{
    int winWidth  = GetScreenWidth();
    int winHeight = GetScreenHeight();

    Color borderColor    = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL));
    int defaultTextColor = GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL);

    Rectangle characterTurnBounds{
        .x      = static_cast<float>(winWidth) - 20.0f,
        .y      = winHeight * 0.1f,
        .width  = 100.0f,
        .height = 100.0f,
    };
    characterTurnBounds.x -= characterTurnBounds.width;

    // Render current turn indicator
    Rectangle currentTurnBounds{
        .x      = characterTurnBounds.x,
        .y      = characterTurnBounds.y + characterTurnBounds.height * m_CurrentTurn,
        .width  = 20.0f,
        .height = 70.0f,
    };
    currentTurnBounds.x -= currentTurnBounds.width;
    currentTurnBounds.y += (characterTurnBounds.height - currentTurnBounds.height) * 0.5f;
    DrawRectangleRec(currentTurnBounds, borderColor);

    // Render Turn order
    for (unsigned i = 0; i < m_TurnOrder.size(); i++)
    {
        const Character* character = m_TurnOrder[i].character;

        // Format turn order button text
        char nameBuffer[MaxTextBuffer];
        auto fmtResult = fmt::format_to_n(nameBuffer,
                                          MaxTextBuffer - 1,
                                          "{}\nHP: {}/{}",
                                          character->GetTypeName(),
                                          character->GetHealth(),
                                          character->GetMaxHealth());
        *fmtResult.out = '\0';

        // Rendering button with text color of character tint
        GuiSetStyle(
            DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(Character::TeamTints[character->GetTeamID()]));
        // GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(character->GetTint()));
        bool result = GuiButton(characterTurnBounds, nameBuffer);
        if (result)
            m_SelectedCharacter = i;

        // Next character in turn order
        characterTurnBounds.y += characterTurnBounds.height;
    }

    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, defaultTextColor);
}

void TurnManager::RenderCharacterActionUI()
{
    RenderCharacterActionPanelUI();

    Rectangle buttonBounds{
        .x      = m_CharAbilitiesRenderBounds.x,
        .y      = m_CharAbilitiesRenderBounds.y + 23,
        .width  = 100.0f,
        .height = 100.0f,
    };

    TurnOrderEntry& entry = m_TurnOrder[m_SelectedCharacter];
    bool isSelectedTurn   = m_CurrentTurn == m_SelectedCharacter;
    bool canExecute       = !entry.character->IsAI() && !m_ExecutingAction && isSelectedTurn;
    if (!canExecute)
        GuiDisable();

    // Actions Buttons
    const std::vector<std::unique_ptr<Action>>& actions = entry.character->GetActions();
    for (unsigned i = 0; i < actions.size(); i++)
    {
        // Format button text
        char textBuffer[MaxTextBuffer];
        unsigned cost = actions[i]->GetCost();
        bool enoughAP = entry.character->GetAvailableAP() >= cost;

        auto fmtResult =
            (cost != Action::CannotPerformCost)
                ? fmt::format_to_n(
                      textBuffer, MaxTextBuffer - 1, "{}\nAP: {}", actions[i]->GetName(), cost)
                : fmt::format_to_n(
                      textBuffer, MaxTextBuffer - 1, "{}\nAP: NA", actions[i]->GetName());
        *fmtResult.out = '\0';

        if (!enoughAP)
            GuiDisable();

        // Render Button
        if (GuiButton(buttonBounds, textBuffer))
        {
            entry.character->SetCurrentAction(i);
            m_ExecutingAction = entry.character->GetCurrentAction()->OnExecute();
        }

        if (canExecute)
            GuiEnable();

        buttonBounds.x += buttonBounds.width;
    }

    // Inventory Buttons
    buttonBounds.x       = m_CharAbilitiesRenderBounds.x + m_CharAbilitiesRenderBounds.width -
                           buttonBounds.width; // right to left
    Inventory& inventory = entry.character->GetInventory();
    const Inventory::UnorderedMap& entries = inventory.GetEntries();
    for (const auto& [itemKey, itemEntry] : entries)
    {
        const auto& [itemDef, count] = itemEntry;

        char textBuffer[MaxTextBuffer];
        auto fmtResult =
            fmt::format_to_n(textBuffer, MaxTextBuffer - 1, "{}\n{}", itemDef.name, count);
        *fmtResult.out = '\0';

        if (canExecute && entry.character->GetSelectedItem() == itemKey)
            GuiDisable();

        // Use item
        if (GuiButton(buttonBounds, textBuffer))
            entry.character->SetSelectedItem(itemKey);

        if (canExecute)
            GuiEnable();

        buttonBounds.x -= buttonBounds.width;
    }

    if (!canExecute)
        GuiEnable();
}

void TurnManager::RenderCharacterActionPanelUI()
{
    int winWidth  = GetScreenWidth();
    int winHeight = GetScreenHeight();

    m_CharAbilitiesRenderBounds = Rectangle{
        .x      = winWidth * 0.5f,
        .y      = static_cast<float>(winHeight) - 20.0f,
        .width  = 900.0f,
        .height = 123.0f,
    };
    m_CharAbilitiesRenderBounds.x -= m_CharAbilitiesRenderBounds.width * 0.5f;
    m_CharAbilitiesRenderBounds.y -= m_CharAbilitiesRenderBounds.height + 3;

    Character* selected = m_TurnOrder[m_SelectedCharacter].character;

    // Format panel text (Character name, current and max AP status)
    char nameBuffer[MaxTextBuffer];
    auto fmtResult = fmt::format_to_n(nameBuffer,
                                      MaxTextBuffer - 1,
                                      "{}, AP: {}/{}",
                                      selected->GetTypeName(),
                                      selected->GetAvailableAP(),
                                      selected->GetMaxAP());
    *fmtResult.out = '\0';

    // Render panel with selected character tint for panel title
    int defaultTextColor = GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(selected->GetTint()));
    GuiPanel(m_CharAbilitiesRenderBounds, nameBuffer);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, defaultTextColor);
}

void TurnManager::IncrementNextTurn()
{
    m_CurrentTurn = (m_CurrentTurn + 1) % m_TurnOrder.size();
    m_TurnOrder[m_CurrentTurn].character->RefreshAbilityPoints();
    m_SelectedCharacter = m_CurrentTurn;
}

} // namespace game
