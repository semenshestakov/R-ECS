#pragma once
#include "../utils/ComponentUtils.hpp"
#include "Components.hpp"


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
    class ComponentsManager final
    {
        friend class Registry;
    public:
        ComponentsManager();
        ~ComponentsManager();

        ComponentsManager(const ComponentsManager& other);
        ComponentsManager& operator=(const ComponentsManager& other);
        void copy(const ComponentsManager& other);

        ComponentsManager(ComponentsManager&& other) noexcept;
        ComponentsManager& operator=(ComponentsManager&& other) noexcept;

    protected:
        void swap(ComponentsManager& other) noexcept;

    public:
        /**
         * @brief Registers a component type with the factory.
         *
         * @tparam ComponentCls The component type to register, must satisfy DerivedComponent concept.
         *
         * @throws error::InvalidComponentId if component ID is invalid.
         * @throws error::RepeatComponent if component ID is already registered.
         *
         * @note Component must have static members: componentId, condition, and default constructor.
         */
        template<DerivedComponent ComponentCls> void Register();

        /**
        * @brief Collects and stores component registration information.
        *
        * @param componentInfo Registration info to store.
        * @throws error::InvalidComponentId if component ID is invalid.
        * @throws error::RepeatComponent if component ID is already registered.
        */
        void Register(const RegisterComponentInfo& componentInfo);

        /**
         * @brief Creates a Components instance with initialized component data.
         *
         * @return Components Instance containing initialized components that pass condition checks.
         * @throws error::InvalidSizeComponents if no valid components are found.
         *
         * @note Only components whose condition evaluates to true with given args are included.
         */
        [[nodiscard]] ComponentsPtr CreateComponents() const;


    private:

        /**
         * @brief Initializes component data buffer and calculates layout.
         *
         * @param[out] componentBuffer Reference to pointer that will hold allocated buffer.
         * @throws error::InvalidSizeComponents if no valid components satisfy conditions.
         *
         * @note Allocates buffer memory that must be managed by Components.
         */
        void initComponentesData(Components*& componentBuffer) const;
#ifdef DEEP_TEST_ENABLE
    public:
#endif
        /// @brief Maximum registered component ID across all registered components.
        componentId_t m_maxRegisteredComponentId {INVALID_COMPONENT_ID};

        /// @brief Array storing registration info for each component ID.
        RegisterComponentInfo m_registeredComponents[OVERFLOW_MAX_COMPONENT_ID]{};

    };

    template<DerivedComponent ComponentCls>
    void ComponentsManager::Register()
    {
        Register({
            .name = typeid(ComponentCls).name(),
            .componentSize=sizeof(ComponentCls),
            .componentId=ComponentCls::componentId,
            .constructor=[](byte* ptr) { new (ptr) ComponentCls(); },
            .destructor=[](byte* ptr) { reinterpret_cast<ComponentCls*>(ptr)->~ComponentCls(); }
        });
    }

}
