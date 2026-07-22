#pragma once

#include "engine/core/ResourceManager.h"
#include "engine/core/Window.h"
#include "engine/graphics/RenderQueue.h"
#include "engine/world/World.h"
#include "raylib.h"

namespace eng {

struct ApplicationCreateInfo
{
    std::string_view title            = "No Name";              // Window title
    WindowResolution windowResolution = WindowResolution::Auto; // Base window resolution
    WindowOptionsFlags windowOptions  = WindowOption_None;      // Things like enabling VSync
    int limit_fps                     = 0;                      // Leave at 0 for monitor Hz rate

    std::string_view backgroundColor = "#000000"; // Clear screen with this color

    std::string_view assetDirectory; // Directory to where all external resources are located
    std::string_view levelFilename;  // External toml level data filename to load
};

class Application
{
public:
    Application(const ApplicationCreateInfo& info);
    virtual ~Application() = default;

    void Run();

protected:
    // Initialize all Actor types that will be used in the game. For example:
    // `world.AddSpawnInfo<CameraController>();`
    virtual void InitializeActorCreateInfos(World& world) = 0;

    // Manually spawn actors using before loading in from level data. For example:
    // `world.Instantiate<CameraController>(position, ...);`
    virtual void OnBeforeLoadingLevelData(World& world) {}

    // Manually spawn actors using after loading in from level data. For example:
    // `world.Instantiate<CameraController>(position, ...);`
    virtual void OnAfterLoadingLevelData(World& world) {}

    ResourceManager& GetResources() { return m_Resources; }

private:
    void LoadSceneData();
    void Render();
    void InitializeRequiredActorsSpawnInfos();

    const std::string_view m_LevelFilename;
    Window m_Window;
    Color m_BackgroundColor;

    ResourceManager m_Resources;
    World m_World;
    RenderQueue m_RenderQueue;
};

} // namespace eng
