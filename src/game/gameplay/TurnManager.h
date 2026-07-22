#pragma once

#include "engine/graphics/Model.h"
#include "engine/world/actors/Actor.h"
#include "engine/world/actors/Tilemap.h"
#include "raylib.h"

// Forward declarations
namespace eng {

class NavigationActor;
class RenderQueue;

} // namespace eng

namespace game {

class TurnManager : public eng::Actor
{
    ENG_ACTOR(TurnManager)

    struct TurnOrderEntry
    {
        class Character* character;

        bool operator>(const TurnOrderEntry& other) const;
    };

public:
    TurnManager(const Vector3& position);
    void OnInitialize(eng::ResourceManager& resources, const eng::SpawnInfo& spawnInfo) override;

    unsigned GetSelectedIndex() const;
    unsigned GetSelectedX() const;
    unsigned GetSelectedY() const;
    unsigned GetSelectedZ() const;

    void RemoveCharacterFromTurnOrder(Character* remove);
    bool IsCharactersTurn(Character* character) const;

    void OnBeginPlay(eng::ResourceManager& resources) override;
    void OnUpdate(float deltaTime) override;
    void SubmitDrawCommands(eng::RenderQueue& renderQueue) override;
    void OnRenderUI() override;

private:
    void RenderTurnOrderUI();
    void RenderCharacterActionUI();
    void RenderCharacterActionPanelUI();

    void SelectCellInTilemap(Vector2 mousePosition);

    void IncrementNextTurn();

    int m_SelectedX;
    int m_SelectedY;
    int m_SelectedZ;
    unsigned m_SelectedIndex;
    const eng::TileDefinition* m_SelectedTileDef = nullptr;
    eng::Model* m_SelectedTileModel;

    std::vector<TurnOrderEntry> m_TurnOrder;
    unsigned m_CurrentTurn       = 0;
    unsigned m_SelectedCharacter = 0;

    Rectangle m_CharAbilitiesRenderBounds;
    bool m_ExecutingAction = false;
};

} // namespace game
