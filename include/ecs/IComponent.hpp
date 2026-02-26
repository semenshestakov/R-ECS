#pragma once
#include "components/ComponentRegistrator.hpp"
#include "registry/RegistryRegistrator.hpp"


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
        friend class ComponentsManager;

        /**
         * @brief Constructs an IComponent and forces registration of the derived component type
         *
         * The (void) cast of IsRegistered ensures the static registration flag is ODR-used,
         * preventing compiler optimization that would otherwise skip the component registration
         * with the ECS registry system. This guarantees the component type is properly
         * registered during static initialization.
         */
        IComponent() {(void)ComponentCls::IsRegistered;}

        /**
         * @brief Unique component type identifier
         *
         * Automatically assigned during static initialization by ComponentRegistrator.
         * Guaranteed to be unique across all component types in the system.
         *
         * @note Initialization order: constructed before IsRegistered
         */
        static inline const componentId_t componentId = ComponentRegistrator::Register<ComponentCls>(typeid(ComponentCls).name());

        /**
         * @brief Factory names for component lookup
         *
         * Array of string identifiers used to register this component with
         * factory systems. Empty by default - can be overridden in derived classes.
         *
         * @note Override in derived class to enable factory-based creation:
         * @code
         * static constexpr std::array<std::string_view, 2> ECS_REGISTRY_NAMES = {
         *     "entity", "scene"
         * };
         * @endcode
         */
        static constexpr std::array<std::string_view, 0> ECS_REGISTRY_NAMES = {};

// Hide registration flag in production, expose in tests
#ifndef DEEP_TEST_ENABLE
    private:
#endif
        /**
         * @brief Component factory registration flag
         *
         * Automatically registers the component with the factory system during
         * static initialization. The registration includes:
         * - Type name (RTTI)
         * - Size in bytes
         * - Unique component ID
         * - Constructor/destructor lambdas
         *
         * @note Initialized after componentId
         * @note Registration occurs exactly once per component type
         * @note Thread-safe initialization
         * @note May throw if registration fails (depends on ComponentsManagerRegistrator)
         */
        [[maybe_unused]] static inline const bool IsRegistered = RegistryRegistrator::RegisterComponent(
            ComponentCls::ECS_REGISTRY_NAMES,
            {
                typeid(ComponentCls).name(),
                sizeof(ComponentCls),
                componentId,
                [](byte* ptr) { new (ptr) ComponentCls(); },
                [](byte* ptr) { reinterpret_cast<ComponentCls*>(ptr)->~ComponentCls(); }
            });


    };

}
