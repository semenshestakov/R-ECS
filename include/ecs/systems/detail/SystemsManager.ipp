#pragma once
#include "ecs/systems/SystemsManager.hpp"


namespace ecs
{
    template<typename SystemCls>
    bool SystemsManager::Register()
    {
        const systemHash_t hash = getSystemHash<SystemCls>();
        if (m_systemsMap.contains(hash))
            return false;

        m_systemsMap[hash] = std::make_unique<SystemCls>();
        m_schedule.Add(*m_systemsMap[hash].get(), hash);
        return true;
    }

    template<typename SystemCls>
    SystemCls& SystemsManager::Get()
    {
        const SystemCls* systemPtr = TryGet<SystemCls>();
        assert(systemPtr != nullptr);
        return *systemPtr;
    }

    template<typename SystemCls>
    const SystemCls& SystemsManager::Get() const
    {
        const SystemCls* systemPtr = TryGet<SystemCls>();
        assert(systemPtr != nullptr);
        return *systemPtr;
    }

    template<typename SystemCls>
    SystemCls* SystemsManager::TryGet()
    {
        const auto it = m_systemsMap.find(getSystemHash<SystemCls>());
        if (it == m_systemsMap.end())
            return nullptr;

        return dynamic_cast<SystemCls*>(it->second.get());
    }

    template<typename SystemCls>
    const SystemCls* SystemsManager::TryGet() const
    {
        const auto it = m_systemsMap.find(getSystemHash<SystemCls>());
        if (it == m_systemsMap.end())
            return nullptr;

        return dynamic_cast<SystemCls*>(it->second.get());
    }

}