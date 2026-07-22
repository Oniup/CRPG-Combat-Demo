#pragma once

#include "engine/core/ResourceManager.h"
#include "engine/utilities/TypeInfo.h"
#include "engine/world/actors/Actor.h"
#include <memory>
#include <type_traits>
#include <unordered_map>

namespace eng {

template <typename T>
concept DerivedActor = std::is_base_of_v<Actor, T>;

class CreateActorRegistry
{
public:
    // Adds Actor spawns static function allow creation from reading level data.
    // NOTE: Actor Type must have SpawnInstance static method defined. This should automatically be
    // applied through ENG_ACTOR macro.
    template <DerivedActor T>
    void Add(ResourceManager& resources)
    {
        static_assert(
            requires(const Vector3& pos) { T::SpawnInstance(pos); },
            "\nAdding an Actor type requires a static ActorType::SpawnInstance(const Vector3& "
            "position) to be defined");

        // Check if Actor type as defined SetupRequiredResources call the function
        if constexpr (requires(ResourceManager& resources) {
                          T::SetupRequiredResources(resources);
                      })
        {
            T::SetupRequiredResources(resources);
        }

        // Get actor hash, used to identify the specific actor to create from level data
        unsigned actorHash                 = TypeInfo<T>::GetHash();
        Actor::Pfn_SpawnInstance spawnFunc = T::SpawnInstance;

        ASSERT(!m_SpawnInfos.contains(actorHash),
               "Cannot have duplicate spawn functions for {}, ID: {}",
               TypeInfo<T>::GetName(),
               actorHash);

        // Add spawn info and call setup_resources if defined
        m_SpawnInfos.emplace(actorHash, spawnFunc);
    }

    // Spawns instance via another Actor's request
    template <typename T>
    std::unique_ptr<Actor> Instantiate(const Vector3& position)
    {
        return Instantiate(TypeInfo<T>::GetHash(), position, TypeInfo<T>::GetName());
    }

    // Spawns an instance of actor with hash code. Spawn functions are defined at program startup.
    // Hash code should be identical to the Actor class TypeInfo<T>::GetHash()
    std::unique_ptr<Actor> Instantiate(unsigned hash, const Vector3& position,
                                       const std::string_view& actorName = "Actor")
    {
        ASSERT(m_SpawnInfos.contains(hash),
               "Registry doesn't contain {} with hash {}",
               actorName,
               hash);

        Actor::Pfn_SpawnInstance spawnFunc = m_SpawnInfos.at(hash);
        return spawnFunc(position);
    }

private:
    std::unordered_map<unsigned, Actor::Pfn_SpawnInstance> m_SpawnInfos;
};

} // namespace eng
