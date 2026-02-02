#pragma once
#include "../RegistryFactoryRegistrator.hpp"


namespace ecs
{

    template<typename T>
    /* static */ RegistryFactoryRegistrator::RegistryFactoryRegistration  RegistryFactoryRegistrator::RegistryFactoryRegistration::create(const std::string& name)
    {
        return RegistryFactoryRegistration(name, std::make_shared<RegistryFactory>());
    }


    /* static */ inline void RegistryFactoryRegistrator::Register(const std::string& name)
    {
        _Registrator::Register<void>(name);
    }

    /* static */ inline std::weak_ptr<RegistryFactory> RegistryFactoryRegistrator::getFactory(const std::string& name) noexcept
    {
        if (RegistryFactoryRegistration *result = _Registrator::get(name))
            return result->factory;
        return {};
    }

}
