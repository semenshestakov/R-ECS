#include <ranges>
#include "ecs/systems/SystemsManager.hpp"


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
        m_schedule.clear();
        m_systemsMap.clear();

        m_schedule = other.m_schedule;
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
        std::swap(m_schedule, other.m_schedule);
    }

    bool SystemsManager::Init(const InitState& state)
    {
        for (auto& system : m_systemsMap | std::views::values)
        {
            if (!system->isInit())
                system->Init(state);
        }
        m_schedule.Init();
        return true;
    }

    void SystemsManager::Update(Registry& registry)
    {
        const UpdateState state {};
        for(const auto& stageSystems : m_schedule)
        {
            for (auto& systemHash : stageSystems)
            {
                m_systemsMap[systemHash]->Update(registry, state);
            }
        }
    }

    std::size_t SystemsManager::size() const
    {
        return m_systemsMap.size();
    }

} // namespace ecs
