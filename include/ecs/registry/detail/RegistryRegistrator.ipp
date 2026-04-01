#pragma once
#include "../RegistryRegistrator.hpp"


template<typename T>
/* static */ ecs::RegistryRegistrator::RegistryInfo
ecs::RegistryRegistrator::RegistryInfo::Create(const std::string& name)
{
    return RegistryInfo(name, SystemsManager(), ComponentsManager());
}

/* static */ inline void ecs::RegistryRegistrator::Register(const std::string& name)
{
    Registrator::Register<void>(name);
}

/* static */ inline ecs::ComponentsManager*
ecs::RegistryRegistrator::GetComponentsManager(const std::string& name) noexcept
{
    if(RegistryInfo* result = Registrator::get(name))
        return &result->componentsManager;
    return nullptr;
}

/* static */ inline ecs::SystemsManager* ecs::RegistryRegistrator::GetSystemsManager(const std::string& name) noexcept
{
    if(RegistryInfo* result = Registrator::get(name))
        return &result->systemManager;
    return nullptr;
}

template<std::size_t N>
/* static */ bool ecs::RegistryRegistrator::RegisterComponent(
    const std::array<std::string_view, N>& names, const RegisterComponentInfo& componentInfo
    )
{
    if constexpr(N == 0)
        return false;

    for(std::string_view nameView: names)
    {
        std::string name = {nameView.begin(), nameView.end()};
        if(!Registrator::contains(name))
            Register(name);

        const auto manager = GetComponentsManager(name);
        manager->Register(componentInfo);
    }

    return true;
}

template<typename System, std::size_t N>
/* static */ bool ecs::RegistryRegistrator::RegisterSystem(const std::array<std::string_view, N>& names)
{
    if constexpr(N == 0)
        return false;

    for(std::string_view nameView: names)
    {
        std::string name = {nameView.begin(), nameView.end()};
        if(!Registrator::contains(name))
            Register(name);

        auto manager = GetSystemsManager(name);
        manager->Register<System>();
    }

    return true;
}
