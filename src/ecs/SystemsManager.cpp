#include "ecs/systems/SystemsManager.hpp"
#include <ranges>


ecs::SystemsManager::SystemsManager() = default;

ecs::SystemsManager::~SystemsManager() = default;

ecs::SystemsManager::SystemsManager(const SystemsManager& other) { this->copy(other); }

ecs::SystemsManager& ecs::SystemsManager::operator=(const SystemsManager& other)
{
    this->copy(other);
    return *this;
}

void ecs::SystemsManager::copy(const SystemsManager& other)
{
    m_schedule.clear();
    m_systemsMap.clear();

    m_schedule = other.m_schedule;
    for(auto const& [hash, systemPtr]: other.m_systemsMap)
    {
        m_systemsMap[hash] = systemPtr_t(systemPtr->New());
    }
}

ecs::SystemsManager::SystemsManager(SystemsManager&& other) noexcept { this->swap(other); }

ecs::SystemsManager& ecs::SystemsManager::operator=(SystemsManager&& other) noexcept
{
    this->swap(other);
    return *this;
}

void ecs::SystemsManager::swap(SystemsManager& other) noexcept
{
    std::swap(m_systemsMap, other.m_systemsMap);
    std::swap(m_schedule, other.m_schedule);
}

bool ecs::SystemsManager::Init(const InitState& state)
{
    for(auto& system: m_systemsMap | std::views::values)
    {
        if(!system->isInit())
            system->Init(state);
    }
    m_schedule.Init();
    return true;
}

void ecs::SystemsManager::Update(Registry& registry)
{
    const UpdateState state{};
    for(const auto& stageSystems: m_schedule)
    {
        for(auto& systemHash: stageSystems)
        {
            m_systemsMap[systemHash]->Update(registry, state);
        }
    }
}

std::size_t ecs::SystemsManager::size() const { return m_systemsMap.size(); }
