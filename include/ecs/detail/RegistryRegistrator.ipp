#pragma once
#include "../RegistryRegistrator.hpp"


namespace ecs
{

    template<typename T>
    /* static */ RegistryRegistrator::RegistryRegistration  RegistryRegistrator::RegistryRegistration::create(const std::string& name)
    {
        return RegistryRegistration(name, std::make_shared<RegistryFactory>());
    }


    /* static */inline void RegistryRegistrator::Register(const std::string& name)
    {
        _Registrator::Register<void>(name);
    }

    /* static */ inline std::weak_ptr<RegistryFactory> RegistryRegistrator::getFactory(const std::string& name) noexcept
    {
        if (RegistryRegistration *result = _Registrator::get(name))
            return result->factory;
        return {};
    }

}
