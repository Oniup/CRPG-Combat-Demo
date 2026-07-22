#include "engine/core/ResourceManager.h"

namespace eng {

ResourceManager::ResourceManager(const std::string_view& assetDirectory)
    : m_AssetDirectory(assetDirectory)
{
}

bool ResourceManager::Contains(const std::string_view& name) const
{
    const auto iter = m_Resources.find(name);
    return iter != m_Resources.end();
}

std::string ResourceManager::GetAssetPath(const std::string_view& path) const
{
    return fmt::format("{}/{}", m_AssetDirectory, path);
}

#ifndef NDEBUG

void ResourceManager::PrintAllResources() const
{
    for (const auto& [id, resource] : m_Resources)
        fmt::println("ID: {}, Type: {}", id, resource->GetInstanceTypeName());
}

#endif

} // namespace eng
