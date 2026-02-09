#pragma once
#include "../RegistryRegistrator.hpp"


namespace ecs
{

    template<typename T>
    /* static */ RegistryRegistrator::RegistryInfo RegistryRegistrator::RegistryInfo::Create(const std::string& name)
    {
        return RegistryInfo(name, ComponentsManager());
    }

    template<std::size_t N>
    bool RegistryRegistrator::RegisterComponent(const std::array<std::string_view, N>& names, const RegisterComponentInfo& componentInfo)
    {
        if constexpr (N == 0)
            return false;

        for (std::string_view nameView : names)
        {
            std::string name = {nameView.begin(), nameView.end()};
            if (!Registrator::contains(name))
                Register(name);

            const auto factory = GetComponentsManager(name);
            factory->Register(componentInfo);
        }

        return true;
    }

    template<std::size_t N>
    bool RegistryRegistrator::RegisterSystem(const std::array<std::string_view, N> &names)
    {
        return true;
    }

    /* static */ inline void RegistryRegistrator::Register(const std::string& name)
    {
        Registrator::Register<void>(name);
    }

    /* static */ inline ComponentsManager* RegistryRegistrator::GetComponentsManager(const std::string& name) noexcept
    {
        if (RegistryInfo *result = Registrator::get(name))
            return &result->factory;
        return nullptr;
    }

}
