#pragma once

#include "engine/core/ResourceManager.h"
#include "engine/world/CreateActorRegistry.h"
#include "engine/world/actors/Actor.h"
#include "engine/world/actors/Tilemap.h"
#include "raylib.h"
#include <memory>
#include <vector>

namespace eng {

class World;
class RenderQueue;

struct RayHitInfo
{
    Actor* hitActor;
    RayCollision rayCollision;

    bool Hit() const { return rayCollision.hit; }
};

// Responsible for loading in the world from a text file and representing the actor data
class World
{
public:
    World(ResourceManager& resources);
    ~World();

public:
    void LoadLevelData(const LevelData& data);
    void Clear();

    void CallActorsOnUpdate(float deltaTime);
    void CallActorsOnRender();
    // TODO: finish Draw Commands
    void CallActorsSubmitDrawCommands(RenderQueue& renderQueue);
    void CallActorsOnRenderUI();

    void SetMainCamera(Camera3D* camera);
    Camera3D* GetMainCamera();
    Tilemap* GetTilemap();

    const Camera3D* GetMainCamera() const;
    const Tilemap* GetTilemap() const;

    RayHitInfo GetRayHit(Actor* self, unsigned targetTagID, Ray ray, bool returnFirst = false);
    RayHitInfo GetRayHit(Actor* self, unsigned targetTagID, const Vector3& startPosition,
                         const Vector3& direction, bool returnFirst = false);
    RayHitInfo GetRayAtMouse(Actor* self, unsigned targetTagID, bool returnFirst = false);

    bool ShouldRenderDebugInfo() const { return m_RenderDebugInfo; }
    void SetRenderDebugInfo(bool shouldRender) { m_RenderDebugInfo = shouldRender; }

    void DestroyActor(Actor* actor);

    template <DerivedActor T>
    void AddSpawnInfo()
    {
        m_Registry.Add<T>(*m_Resources);
    }

    template <DerivedActor T>
    T* Find()
    {
        constexpr unsigned hash = TypeInfo<T>::GetHash();
        return static_cast<T*>(ImplFind(hash, nullptr));
    }

    template <DerivedActor T>
    const T* Find() const
    {
        constexpr unsigned hash = TypeInfo<T>::GetHash();
        return static_cast<T*>(ImplFind(hash, nullptr));
    }

    template <DerivedActor T>
    std::vector<T*> FindAll()
    {
        constexpr unsigned hash = TypeInfo<T>::GetHash();
        unsigned offset         = 0;

        std::vector<T*> actors;
        while (true)
        {
            T* actor = static_cast<T*>(ImplFind(hash, &offset));
            if (!actor)
                break;

            actors.push_back(actor);
        }
        return actors;
    }

    template <DerivedActor T>
    std::vector<const T*> FindAll() const
    {
        constexpr unsigned hash = TypeInfo<T>::GetHash();
        unsigned offset         = 0;

        std::vector<const T*> actors;
        while (true)
        {
            const T* actor = static_cast<T*>(ImplFind(hash, &offset));
            if (!actor)
                break;

            actors.push_back(actor);
        }
        return actors;
    }

    template <DerivedActor T, typename... Args>
    T* Instantiate(const Vector3& position, Args&&... args)
    {
        // Call specific ActorCreateInfo and instantiate actor instance
        const SpawnInfo info{
            .position   = position,
            .definition = nullptr,
        };
        m_DeferredAdd.push_back(m_Registry.Instantiate<T>(info.position));

        // Call OnInitialize
        T* tActor       = static_cast<T*>(m_DeferredAdd.back().get());
        tActor->m_World = this;
        // Only call custom OnInitialize if defined
        if constexpr (requires(T* actor) {
                          actor->OnInitialize(*m_Resources, std::forward<Args>(args)...);
                      })
        {
            tActor->OnInitialize(*m_Resources, std::forward<Args>(args)...);
        }
        else
            tActor->OnInitialize(*m_Resources, info);
        return tActor;
    }

    void PushNewActors();

private:
    void HandleDeferredDestroyedActors();
    Actor* ImplFind(unsigned actorHash, unsigned* offset) const;

    CreateActorRegistry m_Registry;
    ResourceManager* m_Resources;
    std::vector<std::unique_ptr<Actor>> m_Actors;
    std::vector<std::unique_ptr<Actor>> m_DeferredAdd;
    std::vector<Actor*> m_DeferredDestroyed;
    bool m_RenderDebugInfo = false;

    Tilemap* m_Tilemap     = nullptr;
    Camera3D* m_MainCamera = nullptr;
};

} // namespace eng
