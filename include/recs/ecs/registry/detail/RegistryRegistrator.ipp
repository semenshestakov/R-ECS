#pragma once
#include <assert.h>
#include "../RegistryRegistrator.hpp"


template<typename T>
/* static */ ecs::RegistryRegistrator::RegistryInfo ecs::RegistryRegistrator::RegistryInfo::Create(const std::string& name)
{
    return {
        .name = name,
    };
}

/* static */ inline void ecs::RegistryRegistrator::Register(const std::string& name)
{
    if (!Registrator::contains(name))
        Registrator::Register<void>(name);
}

/* static */ inline void ecs::RegistryRegistrator::RegisterSystem(const std::string& name, const std::size_t systemIndex)
{
    RegistryInfo* registryInfo = Registrator::get(name);
    if (registryInfo == nullptr)
    {
        Register(name);
        registryInfo = Registrator::get(name);
    }
    registryInfo->systemRegIndexes.push_back(systemIndex);
}

/* static */ inline const ecs::RegistryRegistrator::RegistryInfo& ecs::RegistryRegistrator::Get(const std::string& name)
{
    const RegistryInfo* registryInfo = Registrator::get(name);
    assert(registryInfo != nullptr);
    return *registryInfo;
}

/* static */ inline bool ecs::RegistryRegistrator::contains(const std::string& name)
{
    return Registrator::contains(name);
}
