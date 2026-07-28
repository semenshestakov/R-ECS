#pragma once
#include <cstdio>
#include "../SystemRegistrator.hpp"
#include "ecs/registry/RegistryRegistrator.hpp"


template<typename T>
ecs::SystemRegistrator::RegisterSystemInfo ecs::SystemRegistrator::RegisterSystemInfo::Create(const std::string& name)
{
    return {
        .name = name,
        .hash = getSystemHash<T>(),
        .makeNew = [](){ return baseSystemPtr_t (new T()); }
    };
}

template<typename SystemCls>
/* static */ ecs::SystemRegistrator::Registrator ecs::SystemRegistrator::Register()
{
    Registrator reg = Registrator::Create<SystemCls>(typeid(SystemCls).name());
    std::fprintf(stderr, "[RECS][reg] system=%s idx=%zu storage=%p slots=%zu\n",
                 typeid(SystemCls).name(), reg.getIndex(), debugStorage(), debugSlots());
    for(const std::string& nameView: SystemCls::GetRegistryNames())
    {
        RegistryRegistrator::RegisterSystem(nameView, reg.getIndex());
    }

    return reg;
}

/* static */ inline const ecs::SystemRegistrator::RegisterSystemInfo* ecs::SystemRegistrator::tryGet(const std::size_t index)
{
    return Registrator::get(index);
}

/* static */ inline const void* ecs::SystemRegistrator::debugStorage() { return Registrator::debugStorage(); }

/* static */ inline std::size_t ecs::SystemRegistrator::debugSlots() { return Registrator::debugSlots(); }

/* static */ inline const ecs::SystemRegistrator::RegisterSystemInfo& ecs::SystemRegistrator::Get(const std::size_t index)
{
    const RegisterSystemInfo* info = tryGet(index);
    assert(info != nullptr);
    return *info;
}
