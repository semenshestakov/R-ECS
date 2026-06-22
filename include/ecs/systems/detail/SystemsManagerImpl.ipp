#pragma once
#include "ecs/systems/SystemsManager.hpp"
#include "ecs/systems/SystemRegistrator.hpp"

inline bool ecs::SystemsManager::Register(const std::size_t systemRegIndex)
{
    const auto& systemInfo = SystemRegistrator::Get(systemRegIndex);
    if (m_systemsMap.contains(systemInfo.hash))
        return false;

    m_systemsMap[systemInfo.hash] = systemInfo.makeNew();
    m_schedule.Add(*m_systemsMap[systemInfo.hash].get(), systemInfo.hash);
    return true;
}

inline ecs::SystemsManager ecs::SystemsManager::Create(const std::span<const std::size_t> systemRegIndexes)
{
    SystemsManager systemsManager;
    for (const std::size_t systemIndex : systemRegIndexes)
        systemsManager.Register(systemIndex);
    return systemsManager;
}
