#pragma once
#include <cassert>
#include "ecs/systems/SystemsManager.hpp"


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

template<typename SystemCls>
void ecs::SystemsManager::Disable()
{
    Disable(getSystemHash<SystemCls>());
}

template<typename SystemCls>
void ecs::SystemsManager::Enable()
{
    Enable(getSystemHash<SystemCls>());
}

template<typename SystemCls>
bool ecs::SystemsManager::IsEnabled() const
{
    return IsEnabled(getSystemHash<SystemCls>());
}
