#pragma once
#include "Registry.hpp"
#include "entities/EntitiesManager.hpp"
#include "registry/RegistryRegistrator.hpp"
#include "systems/SystemUtils.hpp"


namespace ecs
{

    template <typename SystemCls>
    struct ISystem : IBaseSystem
    {
        friend class SystemManager;

        [[nodiscard]] IBaseSystem* New() const override
        {
            return new SystemCls(static_cast<const SystemCls&>(*this));
        }

    private:
        using DerivedSystem = SystemCls;

        static constexpr std::array<std::string_view, 0> SystemManagerNames = {};

        static inline const bool IsRegistered = RegistryRegistrator::RegisterSystem(SystemCls::SystemManagerNames);


    };


}
