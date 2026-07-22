#include "engine/world/World.h"

#include "engine/core/Error.h"
#include "engine/utilities/TypeInfo.h"
#include "engine/world/LevelData.h"
#include "engine/world/actors/Actor.h"
#include "engine/world/actors/Tilemap.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <limits>
#include <memory>

namespace eng {

World::World(ResourceManager& resources)
    : m_Resources(&resources),
      m_Registry()
{
}

World::~World()
{
    Clear();
}

void World::LoadLevelData(const LevelData& data)
{
    // Setup tilemap
    m_DeferredAdd.push_back(m_Registry.Instantiate(
        GetFnvHash(data.tilemapClassName), Vector3Zeros, data.tilemapClassName));

    m_Tilemap = static_cast<Tilemap*>(m_DeferredAdd.back().get());

    ASSERT(m_Tilemap, "Failed to load tilemap with tilemapClass {}", data.tilemapClassName);
    m_Tilemap->m_World = this;
    m_Tilemap->LoadFromLevelData(data, *m_Resources);

    for (const Layer& layer : data.layers)
    {
        for (const Layer::Location& location : layer.actors)
        {
            const TileDefinition* tileDef =
                m_Tilemap->GetTileDefinition(location.x, layer.yIndex, location.z);

            SpawnInfo info{
                .position       = Vector3(location.x, layer.yIndex, location.z),
                .definition     = &data.actorDefinitions.at(location.id),
                .tileDefinition = tileDef,
            };

            const float* actorHeight = info.definition->TryGetField<float>("height");
            if (actorHeight)
                info.position.y -= *actorHeight;
            if (tileDef)
                info.position.y += tileDef->size.y;

            m_DeferredAdd.push_back(m_Registry.Instantiate(info.definition->hash, info.position));
            m_DeferredAdd.back()->m_World = this;
            m_DeferredAdd.back()->OnInitialize(*m_Resources, info);
        }
    }
}

void World::Clear()
{
    for (std::unique_ptr<Actor>& actor : m_Actors)
        actor->OnDestroy();

    m_Actors.clear();
    m_MainCamera = nullptr;
}

void World::CallActorsOnUpdate(float deltaTime)
{
    HandleDeferredDestroyedActors();
    PushNewActors();

    for (unsigned i = 0; i < m_Actors.size(); i++)
        m_Actors[i]->OnUpdate(deltaTime);
}

void World::CallActorsSubmitDrawCommands(RenderQueue& renderQueue)
{
    for (unsigned i = 0; i < m_Actors.size(); i++)
        m_Actors[i]->SubmitDrawCommands(renderQueue);
}

void World::CallActorsOnRenderUI()
{
    for (unsigned i = 0; i < m_Actors.size(); i++)
        m_Actors[i]->OnRenderUI();
}

void World::SetMainCamera(Camera3D* camera)
{
    m_MainCamera = camera;
}

Camera3D* World::GetMainCamera()
{
    return m_MainCamera;
}

Tilemap* World::GetTilemap()
{
    return m_Tilemap;
}

const Camera3D* World::GetMainCamera() const
{
    return m_MainCamera;
}

const Tilemap* World::GetTilemap() const
{
    return m_Tilemap;
}

RayHitInfo World::GetRayHit(Actor* self, unsigned targetTagID, Ray ray, bool returnFirst)
{
    RayHitInfo hitInfo            = {};
    hitInfo.rayCollision.distance = std::numeric_limits<float>::max();

    for (std::unique_ptr<Actor>& actor : m_Actors)
    {
        if (actor.get() == self || (targetTagID != 0 && actor->GetTagID() != targetTagID))
            continue;

        RayCollision collision = actor->CheckRayCollision(ray);
        if (collision.hit)
        {
            if (returnFirst)
                return RayHitInfo{
                    .hitActor     = actor.get(),
                    .rayCollision = collision,
                };

            // Return only the closest
            if (collision.distance < hitInfo.rayCollision.distance)
            {
                hitInfo = RayHitInfo{
                    .hitActor     = actor.get(),
                    .rayCollision = collision,
                };
            }
        }
    }
    return hitInfo;
}

RayHitInfo World::GetRayHit(Actor* self, unsigned targetTagID, const Vector3& startPosition,
                            const Vector3& direction, bool returnFirst)
{
    Ray ray = {
        .position  = startPosition,
        .direction = direction,
    };
    return GetRayHit(self, targetTagID, ray);
}

RayHitInfo World::GetRayAtMouse(Actor* self, unsigned targetTagID, bool returnFirst)
{
    Ray ray = GetScreenToWorldRay(GetMousePosition(), *GetMainCamera());
    return GetRayHit(self, targetTagID, ray, false);
}

void World::DestroyActor(Actor* actor)
{
    m_DeferredDestroyed.push_back(actor);
}

Actor* World::ImplFind(unsigned actorHash, unsigned* offset) const
{
    unsigned begin = offset ? *offset : 0;
    for (unsigned i = begin; i < m_Actors.size(); i++)
    {
        if (m_Actors[i]->GetTargetActorTypeHash() == actorHash)
        {
            if (offset)
                *offset = i + 1;
            return m_Actors[i].get();
        }
    }
    return nullptr;
}

void World::PushNewActors()
{
    if (m_DeferredAdd.empty())
        return;

    unsigned firstAddedIndex = m_Actors.empty() ? 0 : m_Actors.size();
    for (unsigned i = 0; i < m_DeferredAdd.size(); i++)
    {
        m_Actors.push_back(std::move(m_DeferredAdd[i]));
    }
    m_DeferredAdd.clear();

    for (unsigned i = firstAddedIndex; i < m_Actors.size(); i++)
    {
        m_Actors[i]->OnBeginPlay(*m_Resources);
    }
}

void World::HandleDeferredDestroyedActors()
{
    if (m_DeferredDestroyed.empty())
        return;

    unsigned jBegin = 0;
    for (unsigned i = 0; i < m_Actors.size(); i++)
    {
        for (unsigned j = jBegin; j < m_DeferredDestroyed.size(); j++)
        {
            if (m_Actors[i].get() != m_DeferredDestroyed[j])
                continue;

            m_Actors[i]->OnDestroy();
            m_Actors.erase(m_Actors.begin() + i);

            jBegin++;
            break;
        }
        if (jBegin == m_DeferredDestroyed.size())
            break;
    }

    m_DeferredDestroyed.clear();
}

} // namespace eng
