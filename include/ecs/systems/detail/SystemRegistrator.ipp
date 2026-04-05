#pragma once
#include "../SystemRegistrator.hpp"


template<typename T>
ecs::RegisterSystemInfo ecs::RegisterSystemInfo::Create(const std::string& name)
{
    return RegisterSystemInfo(name, SystemsManager());
}

/* static */ inline void ecs::SystemRegistrator::RegisterRegistry(const std::string& name)
{
    Registrator::Register<void>(name);
}

/* static */ inline ecs::SystemsManager* ecs::SystemRegistrator::GetSystemsManager(const std::string& name) noexcept
{
    if(RegisterSystemInfo* result = Registrator::get(name))
        return &result->systemManager;
    return nullptr;
}


template<typename System, std::size_t N>
/* static */ bool ecs::SystemRegistrator::Register(const std::array<std::string_view, N>& names)
{
    if constexpr(N == 0)
        return false;

    for(std::string_view nameView: names)
    {
        std::string name = {nameView.begin(), nameView.end()};
        if(!Registrator::contains(name))
            RegisterRegistry(name);

        auto manager = GetSystemsManager(name);
        manager->Register<System>();
    }

    return true;
}
