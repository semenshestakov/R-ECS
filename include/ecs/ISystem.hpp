#pragma once
#include "registry/RegistryRegistrator.hpp"
#include "systems/IBaseSystem.hpp"


namespace ecs
{

    template <typename SystemCls>
    struct ISystem : IBaseSystem
    {
        using Super = ISystem<SystemCls>;
        friend class SystemsManager;

        ISystem() : IBaseSystem() { }
        [[nodiscard]] IBaseSystem* New() const override { return new SystemCls(static_cast<const SystemCls&>(*this)); }
        [[nodiscard]] const char* name() const override { return typeid(SystemCls).name(); }
        void Init(const InitState& state) override
        {
            IBaseSystem::Init(state);
            (void)(SystemCls::IsRegistered);
        }

#ifndef DEEP_TEST_ENABLE
    private:
#endif
        static constexpr std::array<std::string_view, 0> ECS_REGISTRY_NAMES = {};
        [[maybe_unused]] static inline const bool IsRegistered = RegistryRegistrator::RegisterSystem<SystemCls>(SystemCls::ECS_REGISTRY_NAMES);
    };

}
