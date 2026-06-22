#pragma once
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
    for(const std::string& nameView : SystemCls::GetRegistryNames())
    {
        RegistryRegistrator::RegisterSystem(nameView, reg.getIndex());
    }

    return reg;
}

/* static */ inline const ecs::SystemRegistrator::RegisterSystemInfo& ecs::SystemRegistrator::Get(const std::size_t index)
{
    const RegisterSystemInfo* info = Registrator::get(index);
    assert(info != nullptr);
    return *info;
}
