#pragma once

#include "engine/core/Error.h"
#include "engine/core/Resource.h"
#include "engine/utilities/TypeInfo.h"
#include <functional>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace eng {

template <class T>
concept ResourceClass = std::is_base_of_v<Resource, T>;

class ResourceManager
{

public:
    ResourceManager(const std::string_view& assetDirectory);
    ~ResourceManager() = default;

    const std::string& GetAssetDirectory() const { return m_AssetDirectory; }
    bool Contains(const std::string_view& name) const;

    std::string GetAssetPath(const std::string_view& path) const;

#ifndef NDEBUG
    void PrintAllResources() const;
#endif

    template <ResourceClass T, typename... Args>
    T* Load(const std::string_view& name, Args&&... args)
    {
        DASSERT(!Contains(name),
                "Duplicate resource of type '{}' for key '{}'",
                TypeInfo<T>::GetName(),
                name);

        const auto& [iter, inserted] =
            m_Resources.emplace(name, std::make_unique<T>(std::forward<Args>(args)...));
        return static_cast<T*>(iter->second.get());
    }

    template <ResourceClass T>
    T* Get(const std::string_view& name)
    {
        // exists?
        auto iter = m_Resources.find(name);
        if (iter == m_Resources.end())
            return nullptr;

        std::unique_ptr<Resource>& resource = iter->second;

        // Same type?
        if (resource->GetInstanceTypeHash() != TypeInfo<T>::GetHash())
            return nullptr;

        return static_cast<T*>(resource.get());
    }

    template <ResourceClass T>
    std::vector<T*> GetAll()
    {
        constexpr unsigned hash = TypeInfo<T>::GetHash();

        std::vector<T*> resources;
        for (auto& [_, resource] : m_Resources)
        {
            if (hash == resource->GetInstanceTypeHash())
                resources.push_back(static_cast<T*>(resource.get()));
        }

        return resources;
    }

    template <ResourceClass T>
    std::vector<T*> GetList(const std::initializer_list<std::string_view>& names)
    {
        constexpr unsigned hash = TypeInfo<T>::GetHash();
        std::vector<T*> result;
        result.reserve(names.size());

        for (const std::string_view& name : names)
        {
            auto iter = m_Resources.find(name);
            if (iter != m_Resources.end() && iter->second->GetInstanceTypeHash() == hash)
                result.push_back(static_cast<T*>(iter->second.get()));
            else
                ERROR("Resource {} has not been pushed to ResourceManager", name);
        }

        return result;
    }

private:
    std::string m_AssetDirectory;
    std::unordered_map<std::string, std::unique_ptr<Resource>, StringHash, std::equal_to<>>
        m_Resources;
};

} // namespace eng
