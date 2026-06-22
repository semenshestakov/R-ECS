#pragma once
#include <cassert>
#include <ranges>
#include "ecs/systems/SystemsManager.hpp"

inline ecs::SystemsManager::SystemsManager() = default;
inline ecs::SystemsManager::~SystemsManager() = default;

inline ecs::SystemsManager::SystemsManager(const SystemsManager& other) { this->copy(other); }

inline ecs::SystemsManager& ecs::SystemsManager::operator=(const SystemsManager& other)
{
    this->copy(other);
    return *this;
}

inline void ecs::SystemsManager::copy(const SystemsManager& other)
{
    m_schedule.clear();
    m_systemsMap.clear();

    m_schedule = other.m_schedule;
    for (auto const& [hash, systemPtr] : other.m_systemsMap)
        m_systemsMap[hash] = baseSystemPtr_t(systemPtr->New());
}

inline ecs::SystemsManager::SystemsManager(SystemsManager&& other) noexcept { this->swap(other); }

inline ecs::SystemsManager& ecs::SystemsManager::operator=(SystemsManager&& other) noexcept
{
    this->swap(other);
    return *this;
}

inline void ecs::SystemsManager::swap(SystemsManager& other) noexcept
{
    std::swap(m_systemsMap, other.m_systemsMap);
    std::swap(m_schedule, other.m_schedule);
}

inline bool ecs::SystemsManager::Init(const InitState& state, RegistryToken)
{
    for (const auto& system : m_systemsMap | std::views::values)
    {
        if (!system->isInit())
            system->Init(state);
    }
    m_schedule.Init();
    return true;
}

inline bool ecs::SystemsManager::Subscribe(const SubscribeState& state, RegistryToken)
{
    event::priority_t priority = event::MAX_PRIORITY;
    for (const auto& stageSystems : m_schedule)
    {
        for (auto& systemHash : stageSystems)
        {
            SubscribeState localState = state;
            localState.priority = priority;
            m_systemsMap[systemHash]->Subscribe(localState);
        }
        --priority;
    }
    return true;
}

inline void ecs::SystemsManager::Update(Registry& registry, RegistryToken)
{
    const UpdateState state{};
    for (const auto& stageSystems : m_schedule)
    {
        for (auto& systemHash : stageSystems)
            m_systemsMap[systemHash]->Update(registry, state);
    }
}

inline std::size_t ecs::SystemsManager::size() const
{
    return m_systemsMap.size();
}

template<typename SystemCls>
bool ecs::SystemsManager::Register()
{
    return Register(SystemCls::RegisterInfo.getIndex());
}

template<typename SystemCls>
SystemCls& ecs::SystemsManager::Get()
{
    SystemCls* systemPtr = TryGet<SystemCls>();
    assert(systemPtr != nullptr);
    return *systemPtr;
}

template<typename SystemCls>
const SystemCls& ecs::SystemsManager::Get() const
{
    const SystemCls* systemPtr = TryGet<SystemCls>();
    assert(systemPtr != nullptr);
    return *systemPtr;
}

template<typename SystemCls>
SystemCls* ecs::SystemsManager::TryGet()
{
    const auto it = m_systemsMap.find(getSystemHash<SystemCls>());
    if(it == m_systemsMap.end())
        return nullptr;

    return dynamic_cast<SystemCls*>(it->second.get());
}

template<typename SystemCls>
const SystemCls* ecs::SystemsManager::TryGet() const
{
    const auto it = m_systemsMap.find(getSystemHash<SystemCls>());
    if(it == m_systemsMap.end())
        return nullptr;

    return dynamic_cast<const SystemCls*>(it->second.get());
}
