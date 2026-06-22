#pragma once
#include "ecs/Registry.hpp"
#include "ecs/registry/RegistryRegistrator.hpp"
#include "ecs/systems/detail/SystemsManagerImpl.ipp"

inline ecs::Registry::Registry(SystemsManager systemManager) :
    m_systemManager(std::move(systemManager)),
    m_eventSystem(*this)
{}

inline ecs::Registry::Registry() :
    m_eventSystem(*this)
{}

inline bool ecs::Registry::Init(void* args)
{
    const bool result = m_systemManager.Init({"", args}, {});
    if (result)
        return result & m_systemManager.Subscribe({m_eventSystem}, {});
    return result;
}

inline void ecs::Registry::Update()
{
    m_commandQueue.Flush({}, *this);
    m_systemManager.Update(*this, {});
    m_eventSystem.FlushEvents({});
    m_commandQueue.Flush({}, *this);
}

inline ecs::Registry ecs::Registry::Create(const std::string& name)
{
    return Registry{SystemsManager::Create(RegistryRegistrator::Get(name).systemRegIndexes)};
}
