#include "engine/core/Application.h"

#include "engine/graphics/Model.h"
#include "engine/utilities/Color.h"
#include "engine/utilities/DebugRayActor.h"
#include "engine/world/LevelData.h"
#include "engine/world/actors/Tilemap.h"
#include "raylib.h"
#include <cmath>
#include <fmt/std.h>

namespace eng {

#ifndef NDEBUG
void PrintLevelData(const LevelData& data)
{
    fmt::println("width: {}, height: {}, name: {}", data.width, data.length, data.name);

    fmt::println("\nTile definitions");
    for (const auto& [id, definition] : data.tileDefinitions)
    {
        fmt::println("{}, {} ({}):", id, definition.name, definition.fields.size());
        for (const auto& [field, value] : definition.fields)
            fmt::println("{} = {}", field, value);
    }

    fmt::println("\nTile definitions");
    for (const auto& [id, definition] : data.actorDefinitions)
    {
        fmt::println("{}, {} ({}):", id, definition.name, definition.fields.size());
        for (const auto& [field, value] : definition.fields)
            fmt::println("{} = {}", field, value);
    }

    fmt::println("\nLayer definitions ({}):", data.layers.size());
    for (const Layer& layer : data.layers)
    {
        fmt::println("\nz_index {}:\nTerrain:", layer.yIndex);

        for (unsigned i = 0; i < layer.terrain.size(); i++)
        {
            int x = i % data.width;
            int z = std::floor(i / data.width);
            fmt::println("({}, {}) = {}", x, z, static_cast<char>(layer.terrain[i]));
        }

        fmt::println("Actors:");
        for (const Layer::Location& location : layer.actors)
            fmt::println("({}, {}) = {}", location.x, location.z, static_cast<char>(location.id));
    }
}
#endif

Application::Application(const ApplicationCreateInfo& info)
    : m_Window(info.title, info.limit_fps, info.windowResolution, info.windowOptions),
      m_BackgroundColor(ConvertHexToColor(info.backgroundColor)),
      m_Resources(info.assetDirectory),
      m_LevelFilename(info.levelFilename),
      m_World(m_Resources)
{
    m_Resources.Load<Model>("Cube", Model::CreateCube());
    m_Resources.Load<Model>("Plane", Model::CreatePlane());
    m_Resources.Load<Model>("Cylinder", Model::CreateCylinder());
}

void Application::Run()
{
    // Initialize world
    InitializeRequiredActorsSpawnInfos();
    InitializeActorCreateInfos(m_World);
    LoadSceneData();
    m_World.PushNewActors();

    ASSERT(m_World.GetMainCamera(), "Actor must contain a camera and set it to world");

    float lastTime = 0.0f;
    while (m_Window.IsOpen())
    {
        // Calculate delta time
        float time      = static_cast<float>(GetTime());
        float deltaTime = time - lastTime;
        lastTime        = time;

        if (IsKeyPressed(KEY_ESCAPE))
            m_World.SetRenderDebugInfo(!m_World.ShouldRenderDebugInfo());

        // Update loop
        m_World.CallActorsOnUpdate(deltaTime);
        Render();
    }
}

void Application::LoadSceneData()
{
    LevelData data(m_Resources.GetAssetPath(m_LevelFilename));

    OnBeforeLoadingLevelData(m_World);
    m_World.LoadLevelData(data);
    OnAfterLoadingLevelData(m_World);
}

void Application::Render()
{
    m_World.CallActorsSubmitDrawCommands(m_RenderQueue);

    BeginDrawing();
    ClearBackground(m_BackgroundColor);
    {
        // Render World
        m_RenderQueue.Render(*m_World.GetMainCamera());

        // Draw UI
        DrawFPS(0.0f, 0.0f);
        m_World.CallActorsOnRenderUI();
    }
    EndDrawing();

    m_RenderQueue.Clear();
}

void Application::InitializeRequiredActorsSpawnInfos()
{
    m_World.AddSpawnInfo<Tilemap>();
    m_World.AddSpawnInfo<DebugRayActor>();
}

} // namespace eng
