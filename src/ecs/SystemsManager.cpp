#include "ecs/systems/SystemsManager.hpp"

#include <ranges>


namespace ecs
{

    SystemsManager::SystemsManager() = default;

    SystemsManager::~SystemsManager() = default;

    SystemsManager::SystemsManager(const SystemsManager& other)
    {
        this->copy(other);
    }

    SystemsManager &SystemsManager::operator=(const SystemsManager& other)
    {
        this->copy(other);
        return *this;
    }

    void SystemsManager::copy(const SystemsManager& other)
    {
        m_updates.clear();
        m_systemsMap.clear();

        m_updates = other.m_updates;
        for (auto const& [hash, systemPtr] : other.m_systemsMap)
        {
            m_systemsMap[hash] = systemPtr_t(systemPtr->New());
        }
    }

    SystemsManager::SystemsManager(SystemsManager&& other) noexcept
    {
        this->swap(other);
    }
    
    SystemsManager& SystemsManager::operator=(SystemsManager&& other) noexcept
    {
        this->swap(other);
        return *this;
    }
    
    void SystemsManager::swap(SystemsManager& other) noexcept
    {
        std::swap(m_systemsMap, other.m_systemsMap);
        std::swap(m_updates, other.m_updates);
    }

    bool SystemsManager::Init(const InitState& state)
    {
        for (auto& system : m_systemsMap | std::views::values)
        {
            if (!system->isInit())
                system->Init(state);
        }
        return true;
    }

    void SystemsManager::Update(Registry& registry, const std::optional<updateTag_t> updateTag /* = nullopt */)
    {
        UpdateState state {};
        if (updateTag == std::nullopt)
        {
            for (const auto& vectorHash : m_updates | std::views::values)
            {
                for (const hash_t systemHash : vectorHash)
                {
                    if (IBaseSystem& system = *m_systemsMap.find(systemHash)->second; system.isInit())
                        m_systemsMap.find(systemHash)->second->Update(registry, state);
                }
            }
        }
        else if (m_updates.contains(updateTag.value()))
        {
            for (const hash_t systemHash : m_updates[updateTag.value()])
            {
                if (IBaseSystem& system = *m_systemsMap.find(systemHash)->second; system.isInit())
                    m_systemsMap.find(systemHash)->second->Update(registry, state);
            }
        }
    }

    std::size_t SystemsManager::size() const
    {
        return m_systemsMap.size();
    }

} // namespace ecs
