#pragma once
#include "common_recs/utils/ClassUtils.hpp"
#include "registry/RegistryRegistrator.hpp"
#include "systems/IBaseSystem.hpp"
#include "event/Listener.hpp"
#include "event/EventSystem.hpp"


namespace ecs
{

    template <typename SystemCls>
    struct ISystem : IBaseSystem
    {
    protected:
        friend class SystemsManager;
        using Super = ISystem<SystemCls>;
        using SelfSystemCls = SystemCls;
        template <class Event>
        using EventListener = event::Listener<event::callbackId_t, Registry&, const Event&>;

    public:
        ISystem() : IBaseSystem() { }

        [[nodiscard]] IBaseSystem* New() const override { return new SystemCls(); }
        [[nodiscard]] const char* name() const override { return typeid(SystemCls).name(); }
        void Init(const InitState& state) override;

    protected:
        template <class Event>
        auto RegisterEvent(void (SystemCls::*method)(Registry&, const Event&)) -> std::unique_ptr<EventListener<Event>>;

    DEEP_TEST_PRIVATE_ACCESS:
        static constexpr std::array<std::string_view, 0> ECS_REGISTRY_NAMES = {};
        [[maybe_unused]] static inline const bool IsRegistered = RegistryRegistrator::RegisterSystem<SystemCls>(SystemCls::ECS_REGISTRY_NAMES);
        std::vector<std::function<void(EventSystem&)>> m_registerEventFunctions;
    };

} // namespace ecs

#define ECS_EVENT(_METHOD_NAME, _EVENT) \
    private: \
    std::unique_ptr<Super::EventListener<_EVENT>> m_listener_##_EVENT = Super::RegisterEvent<_EVENT>(&SelfSystemCls::_METHOD_NAME); \
    public:

#include "systems/detail/ISystem.ipp"
