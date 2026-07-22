#pragma once

#include "engine/core/Application.h"

namespace game {

class Game : public eng::Application
{
public:
    static Game CreateInstance(int argc, char** argv);

    Game(const eng::ApplicationCreateInfo& createInfo);

protected:
    void InitializeActorCreateInfos(eng::World& world) override;
    void OnAfterLoadingLevelData(eng::World& world) override;

private:
    static std::string GetAssetDirectory();
};

} // namespace game
