#include "ecs/systems/SystemManager.hpp"

#include <ranges>


namespace ecs
{

    SystemManager::SystemManager() = default;

    SystemManager::~SystemManager() = default;

    SystemManager::SystemManager(const SystemManager &other)
    {
        this->copy(other);
    }

    SystemManager &SystemManager::operator=(const SystemManager &other)
    {
        this->copy(other);
        return *this;
    }

    void SystemManager::copy(const SystemManager& other)
    {
        m_updates.clear();
        m_systemsMap.clear();

        m_updates = other.m_updates;
        for (auto const& [hash, systemPtr] : other.m_systemsMap)
        {
            m_systemsMap[hash] = systemPtr_t(systemPtr->New());
        }
    }

    SystemManager::SystemManager(SystemManager &&other) noexcept
    {
        this->swap(other);
    }
    
    SystemManager& SystemManager::operator=(SystemManager &&other) noexcept
    {
        this->swap(other);
        return *this;
    }
    
    void SystemManager::swap(SystemManager& other) noexcept
    {
        std::swap(m_systemsMap, other.m_systemsMap);
        std::swap(m_updates, other.m_updates);
    }

    void SystemManager::Update(Registry& registry, const std::optional<updateTag_t> updateTag /* = nullopt */)
    {
        if (updateTag == std::nullopt)
        {
            for (const auto& vectorHash : m_updates | std::views::values)
            {
                for (const hash_t systemHash : vectorHash)
                {
                    UpdateState state {};
                    m_systemsMap.find(systemHash)->second->Update(registry, state);
                }
            }
        }
        else if (m_updates.contains(updateTag.value()))
        {
            for (const hash_t systemHash : m_updates[updateTag.value()])
            {
                UpdateState state {};
                m_systemsMap.find(systemHash)->second->Update(registry, state);
            }
        }
    }

} // namespace ecs
