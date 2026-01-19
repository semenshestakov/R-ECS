#pragma once
#include "Components.hpp"
#include "utils/ComponentUtils.hpp"


namespace ecs
{

    /**
     * @brief Factory class for creating Components with registered component types.
     *
     * The ComponentsFactory manages the registration of component types and
     * creates Components instances that contain initialized component data
     * based on conditional checks.
     *
     * @note This class is non-copyable and non-movable.
     */
    class RegistryFactory
    {
    public:
        RegistryFactory();
        ~RegistryFactory();

        // Delete copy and move operations to enforce singleton-like behavior
        RegistryFactory(RegistryFactory&&) noexcept = delete;
        RegistryFactory& operator=(RegistryFactory&&) noexcept = delete;
        RegistryFactory(const RegistryFactory&) = delete;
        RegistryFactory& operator=(const RegistryFactory&) = delete;

        /**
         * @brief Registers a component type with the factory.
         *
         * @tparam ComponentCls The component type to register, must satisfy BaseOfComponents concept.
         *
         * @throws error::InvalidComponentId if component ID is invalid.
         * @throws error::RepeatComponent if component ID is already registered.
         *
         * @note Component must have static members: componentId, condition, and default constructor.
         */
        template<BaseOfComponents ComponentCls> void Register();

        /**
         * @brief Creates a Components instance with initialized component data.
         *
         * @param args Optional condition arguments passed to component condition checks.
         * @return Components Instance containing initialized components that pass condition checks.
         * @throws error::InvalidSizeComponents if no valid components are found.
         *
         * @note Only components whose condition evaluates to true with given args are included.
         */
        [[nodiscard]] ComponentsPtr createComponents() const;

    private:

        /**
         * @brief Collects and stores component registration information.
         *
         * @param componentInfo Registration info to store.
         * @throws error::InvalidComponentId if component ID is invalid.
         * @throws error::RepeatComponent if component ID is already registered.
         */
        void collectRegisterComponentInfo(RegisterComponentInfo&& componentInfo);

        /**
         * @brief Initializes component data buffer and calculates layout.
         *
         * @param[out] componentBuffer Reference to pointer that will hold allocated buffer.
         * @throws error::InvalidSizeComponents if no valid components satisfy conditions.
         *
         * @note Allocates buffer memory that must be managed by Components.
         */
        void initComponentesData(Components*& componentBuffer) const;

        /// @brief Maximum registered component ID across all registered components.
        componentId_t m_maxRegisteredComponentId {INVALID_COMPONENT_ID};

        /// @brief Array storing registration info for each component ID.
        RegisterComponentInfo m_registeredComponents[OVERFLOW_MAX_COMPONENT_ID]{};

    };

    template<BaseOfComponents ComponentCls>
    void RegistryFactory::Register()
    {
        collectRegisterComponentInfo({
            .componentSize=sizeof(ComponentCls),
            .componentId=ComponentCls::componentId,
            .constructor=[](byte* ptr) { new (ptr) ComponentCls(); },
            .destructor=[](byte* ptr) { reinterpret_cast<ComponentCls*>(ptr)->~ComponentCls(); }
        });
    }

}
