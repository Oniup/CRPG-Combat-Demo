#pragma once

#include "engine/utilities/TypeInfo.h"
#include "raylib.h"
#include "raymath.h"
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#define ENG_ACTOR(actor_class)                                                                     \
public:                                                                                            \
    static std::unique_ptr<Actor> SpawnInstance(const Vector3& position)                           \
    {                                                                                              \
        return std::make_unique<actor_class>(position);                                            \
    }                                                                                              \
    unsigned GetTargetActorTypeHash() const override                                               \
    {                                                                                              \
        return eng::TypeInfo<actor_class>::GetHash();                                              \
    }                                                                                              \
    const std::string_view GetTargetActorTypeName() const override                                 \
    {                                                                                              \
        return eng::TypeInfo<actor_class>::GetName();                                              \
    }                                                                                              \
                                                                                                   \
private:

namespace eng {

class World;
class RenderQueue;
class ResourceManager;
struct LevelTypeDef;
struct TileDefinition;

struct SpawnInfo
{
    Vector3 position                     = Vector3Zeros;
    const LevelTypeDef* definition       = nullptr;
    const TileDefinition* tileDefinition = nullptr;
};

class Actor
{
    friend World;

public:
    using Pfn_SpawnInstance = std::function<std::unique_ptr<Actor>(const Vector3& position)>;
    using Pfn_SetupRequiredResources = std::function<void(ResourceManager& resources)>;

    Vector3 position = Vector3Zeros;

    static constexpr unsigned GetTagID(const std::string_view& tag) { return GetFnvHash(tag); }

    virtual ~Actor() = default;

    // Called when adding
    virtual void OnInitialize(ResourceManager& resources, const SpawnInfo& info) {}

    // Called just before the first frame is calculated. this ensures that all actors are loaded
    // into the world.
    virtual void OnBeginPlay(ResourceManager& resources) {}

    // Called when the actor is being removed from the world.
    virtual void OnDestroy() {}

    // Called once per frame to update the actor’s state.
    virtual void OnUpdate(float deltaTime) {}

    // Called when the engine is ready to render the actor.
    virtual void SubmitDrawCommands(RenderQueue& renderQueue) {}

    // Called when the engine is ready to render UI defined by the actor
    virtual void OnRenderUI() {}

    // Called to check if the ray hits the object
    virtual RayCollision CheckRayCollision(const Ray& ray) const { return RayCollision{}; }

    // Id used to identify distinct actor groups from other actors of the same type. Use
    // `GetFnvHash` function to translate the string name into an unsigned id
    unsigned GetTagID() const { return GetFnvHash(m_Tag); }
    const std::string& GetTag() const { return m_Tag; }

    // Get pointer to world that holds all actors
    World* GetWorld() { return m_World; }
    const World* GetWorld() const { return m_World; }

    // Generated through ENG_ACTOR macro
    virtual unsigned GetTargetActorTypeHash() const               = 0;
    virtual const std::string_view GetTargetActorTypeName() const = 0;

protected:
    Actor(Vector3 position, std::string&& tag)
        : position(position),
          m_Tag(std::move(tag))
    {
    }

    Actor(Vector3 position)
        : position(position),
          m_Tag()
    {
    }

    void SetTag(const std::string& tag) { m_Tag = tag; }
    void SetTag(std::string&& tag) { m_Tag = std::move(tag); }

private:
    World* m_World = nullptr;
    std::string m_Tag;
};

} // namespace eng
