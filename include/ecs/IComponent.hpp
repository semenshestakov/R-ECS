#pragma once
#include "ComponentRegistrator.hpp"


namespace ecs
{
    /**
     * @brief Base class for all ECS components
     *
     * Provides the fundamental interface and identification mechanism
     * for all components in the Entity Component System. Each derived
     * component class must define its own unique componentId.
     */
    template <typename ComponentCls>
    struct IComponent
    {
#ifndef ECS_TESTING
    private:
#endif
        friend class RegistryFactory;
        friend struct RegisterComponentInfo;

        static inline const ecs::componentId_t componentId = ecs::ComponentRegistrator::Register<ComponentCls>(typeid(ComponentCls).name());
    };
}
