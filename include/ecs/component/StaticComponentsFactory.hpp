#pragma once
#include "Component.hpp"
#include "StaticComponents.hpp"


namespace ecs::component
{

    class StaticComponentsFactory
    {
    public:
        StaticComponentsFactory();
        ~StaticComponentsFactory();

        StaticComponentsFactory(StaticComponentsFactory&& other) noexcept = delete;
        StaticComponentsFactory& operator=(StaticComponentsFactory&& other) noexcept = delete;
        StaticComponentsFactory(const StaticComponentsFactory&) = delete;
        StaticComponentsFactory& operator=(const StaticComponentsFactory&) = delete;

        template<BaseOfComponents COMPONENT> void Register();
        StaticComponents&& createComponent(const BaseComponent::ConditionArgs* args) const;

    private:
        void collectRegisterComponentInfo(StaticComponents::RegisterComponentInfo&& componentInfo);

        componentId_t m_maxRegisteredComponentId {INVALID_COMPONENT_ID};
        StaticComponents::RegisterComponentInfo m_registeredComponents[OVERFLOW_MAX_COMPONENT_ID]{};

    };

    template<BaseOfComponents COMPONENT>
    void StaticComponentsFactory::Register()
    {
        collectRegisterComponentInfo({
            .componentSize=sizeof(COMPONENT),
            .componentId=COMPONENT::componentId,
            .condition=COMPONENT::condition,
            .constructor=[](byte* ptr) { new (ptr) COMPONENT(); }
        });
    }

}
