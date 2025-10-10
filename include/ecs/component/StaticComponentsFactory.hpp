#pragma once

#include "Component.hpp"
#include "StaticComponents.hpp"


namespace ecs::component
{

    /**
     * @brief Factory class for creating StaticComponents with registered component types.
     *
     * The StaticComponentsFactory manages the registration of component types and
     * creates StaticComponents instances that contain initialized component data
     * based on conditional checks.
     *
     * @note This class is non-copyable and non-movable.
     */
    class StaticComponentsFactory
    {
    public:
        StaticComponentsFactory();
        ~StaticComponentsFactory();

        // Delete copy and move operations to enforce singleton-like behavior
        StaticComponentsFactory(StaticComponentsFactory&&) noexcept = delete;
        StaticComponentsFactory& operator=(StaticComponentsFactory&&) noexcept = delete;
        StaticComponentsFactory(const StaticComponentsFactory&) = delete;
        StaticComponentsFactory& operator=(const StaticComponentsFactory&) = delete;

        /**
         * @brief Registers a component type with the factory.
         *
         * @tparam COMPONENT The component type to register, must satisfy BaseOfComponents concept.
         *
         * @throws error::InvalidComponentId if component ID is invalid.
         * @throws error::RepeatComponent if component ID is already registered.
         *
         * @note Component must have static members: componentId, condition, and default constructor.
         */
        template<BaseOfComponents COMPONENT> void Register();

        /**
         * @brief Creates a StaticComponents instance with initialized component data.
         *
         * @param args Optional condition arguments passed to component condition checks.
         * @return StaticComponents Instance containing initialized components that pass condition checks.
         * @throws error::InvalidSizeComponents if no valid components are found.
         *
         * @note Only components whose condition evaluates to true with given args are included.
         */
        StaticComponentsPtr createComponents(const BaseComponent::ConditionArgs* args = nullptr) const;

    private:

        /**
         * @brief Collects and stores component registration information.
         *
         * @param componentInfo Registration info to store.
         * @throws error::InvalidComponentId if component ID is invalid.
         * @throws error::RepeatComponent if component ID is already registered.
         */
        void collectRegisterComponentInfo(StaticComponents::RegisterComponentInfo&& componentInfo);

        /**
         * @brief Initializes component data buffer and calculates layout.
         *
         * @param[out] componentBuffer Reference to pointer that will hold allocated buffer.
         * @param args Optional condition arguments for component filtering.
         * @throws error::InvalidSizeComponents if no valid components satisfy conditions.
         *
         * @note Allocates buffer memory that must be managed by StaticComponents.
         */
        void initComponentesData(StaticComponents*& componentBuffer, const BaseComponent::ConditionArgs* args = nullptr) const;

        /// @brief Maximum registered component ID across all registered components.
        componentId_t m_maxRegisteredComponentId {INVALID_COMPONENT_ID};

        /// @brief Array storing registration info for each component ID.
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
