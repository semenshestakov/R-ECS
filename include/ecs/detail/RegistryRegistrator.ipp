#pragma once
#include "../RegistryFactoryRegistrator.hpp"

namespace ecs
{

    template<typename T>
    /* static */ RegistryFactoryRegistrator::RegistryFactoryRegistration RegistryFactoryRegistrator::RegistryFactoryRegistration::create(const std::string& name)
    {
        return RegistryFactoryRegistration(name, RegistryFactory());
    }

    template<std::size_t N>
    bool RegistryFactoryRegistrator::RegisterComponent(const std::array<std::string_view, N>& names, const RegisterComponentInfo& componentInfo)
    {
        if constexpr (N == 0)
            return false;

        for (std::string_view nameView : names)
        {
            std::string name = {nameView.begin(), nameView.end()};
            if (!Registrator::contains(name))
                Register(name);

            const auto factory = GetFactory(name);
            factory->Register(componentInfo);
        }

        return true;
    }

    /* static */ inline void RegistryFactoryRegistrator::Register(const std::string& name)
    {
        Registrator::Register<void>(name);
    }

    /* static */ inline RegistryFactory* RegistryFactoryRegistrator::GetFactory(const std::string& name) noexcept
    {
        if (RegistryFactoryRegistration *result = Registrator::get(name))
            return &result->factory;
        return nullptr;
    }

}
