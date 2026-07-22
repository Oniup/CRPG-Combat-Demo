#include "main/Game.h"

#include "engine/core/Window.h"
#include "environment/Barrel.h"
#include "gameplay/CameraController.h"
#include "gameplay/Character.h"
#include "gameplay/TurnManager.h"
#include "main/MoveWeightedTilemap.h"
#include "raygui.h"
#include "raylib.h"
#include "raymath.h"
#include <filesystem>

namespace game {

Game Game::CreateInstance(int argc, char** argv)
{
    std::string assetDirectory = GetAssetDirectory();

    return Game(eng::ApplicationCreateInfo{

        .title           = "Path Finding",
        .windowOptions   = eng::WindowOption_ManualResizable,
        .backgroundColor = "#222222",

        .assetDirectory = assetDirectory,
        .levelFilename  = "maps/MainLevel.toml",
        // .levelFilename  = "maps/InfluenceMapTest.toml",
    });
}

Game::Game(const eng::ApplicationCreateInfo& createInfo)
    : eng::Application(createInfo)
{
    GuiLoadStyle(GetResources().GetAssetPath("style_jungle.rgs").c_str());
    Font font     = GuiGetFont();
    font.baseSize = font.baseSize * 0.7f;
    GuiSetFont(font);
}

void Game::InitializeActorCreateInfos(eng::World& world)
{
    // Controllers
    world.AddSpawnInfo<CameraController>();
    world.AddSpawnInfo<TurnManager>();

    // Environment
    world.AddSpawnInfo<Barrel>();
    world.AddSpawnInfo<MoveWeightedTilemap>();

    // Characters
    world.AddSpawnInfo<Character>();
}

void Game::OnAfterLoadingLevelData(eng::World& world)
{
    world.Instantiate<CameraController>(Vector3Zeros, CAMERA_PERSPECTIVE, 10.0f, 45.0f);
    world.Instantiate<TurnManager>(Vector3Zeros);
}

std::string Game::GetAssetDirectory()
{
    namespace fs = std::filesystem;

    fs::path workingDirectory = fs::current_path();
    while (workingDirectory.has_parent_path())
    {
        auto enumerateDirectories = fs::directory_iterator(workingDirectory);

        for (const fs::directory_entry& dir : enumerateDirectories)
            if (dir.is_directory() && dir.path().filename() == "assets")
                return dir.path().string();

        workingDirectory = workingDirectory.parent_path();
    }

    ASSERT(false, "Failed to find working directory");
}

} // namespace game
