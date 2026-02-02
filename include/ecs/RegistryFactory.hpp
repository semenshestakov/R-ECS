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
    class RegistryFactory final
    {
        friend class AbstractRegistry;
    public:
        RegistryFactory();
        ~RegistryFactory();

    protected:
        RegistryFactory(const RegistryFactory& other);
        RegistryFactory& operator=(const RegistryFactory& other);
        void copy(const RegistryFactory& other);

        RegistryFactory(RegistryFactory&& other) noexcept;
        RegistryFactory& operator=(RegistryFactory&& other) noexcept;
        void swap(RegistryFactory& other) noexcept;

    public:
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
