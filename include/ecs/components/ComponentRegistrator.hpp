#ifndef COMPONENTS_REGISTRATOR_HPP
#define COMPONENTS_REGISTRATOR_HPP

#include "../utils/ComponentUtils.hpp"
#include "reg/Registrator.hpp"


namespace ecs
{

    /**
     * @brief Metadata for component registration and construction.
     */
    struct RegisterComponentInfo final
    {
        std::string name;
        bufferSize_t componentSize {0};                             ///< Size of the component in bytes
        componentId_t componentId {INVALID_COMPONENT_ID};           ///< Unique identifier for the component
        void(*constructor)(byte*) = nullptr;                        ///< Placement new constructor function
        void(*destructor)(byte*) = nullptr;                         ///< Destructor function

        template <typename ComponentCls>
        static RegisterComponentInfo Create(const std::string& name);
    };

    /**
     * @brief Component registration manager for the ECS framework.
     *
     * The ComponentRegistrator class provides static methods to register component types
     * with the ECS system. It manages the assignment of unique component IDs and maintains
     * registration information for each component type.
     *
     * @note This class uses protected inheritance from reg::Registrator to extend
     *       registration functionality while maintaining encapsulation.
     * @note Component IDs start from 1 (0 is reserved for invalid/unregistered components).
     *
     * @tparam ComponentCls The component type to register.
     */
    class ComponentRegistrator final : protected reg::Registrator<RegisterComponentInfo>
    {
        using Super = reg::Registrator<RegisterComponentInfo>;
    public:
        template <typename ComponentCls>

        /**
         * @brief Registers a component type and assigns it a unique ID.
         *
         * Registers the specified component type with the ECS system. Each registered
         * component receives a unique numeric identifier that can be used throughout
         * the ECS framework to reference this component type.
         *
         * The registration process:
         * 1. Creates a registration entry for the component type
         * 2. Assigns a unique component ID (index + 1)
         * 3. Stores the ID in the registration metadata
         * 4. Returns the assigned ID for immediate use
         *
         * @tparam ComponentCls Type of component to register
         * @param name Name to identify the component (typically from typeid().name() or a string literal)
         *
         * @return componentId_t Unique ID assigned to this component type
         *
         * @note Component IDs start from 1 (0 is reserved for invalid/unregistered)
         * @note Registration is idempotent - calling multiple times returns the same ID
         */
        static componentId_t Register(const std::string& name);


        static const RegisterComponentInfo& GetInfo(componentId_t componentId);
    };

}
#endif
#include "detail/ComponentRegistrator.ipp"
