#pragma once
#include "reg/Registrator.hpp"
#include "RegistryFactory.hpp"

namespace ecs
{

    /**
     * @brief Manages registration and retrieval of RegistryFactory instances.
     *
     * The RegistryRegistrator provides a central registry for creating and accessing
     * RegistryFactory objects by name. It uses a unique registration strategy
     * ensuring that each registry name is registered only once throughout the
     * application lifetime.
     *
     * @note Uses RegistrationStrategy::UNIQUE - each name can have only one factory
     * @note Returns std::weak_ptr to factories to avoid ownership cycles
     * @see reg::Registrator
     * @see RegistryFactory
     */
    class RegistryFactoryRegistrator final
    {
#ifdef DEEP_TESTS_TEST_ENABLE
    public:
#endif
        /**
         * @brief Internal structure storing registry metadata.
         *
         * Contains registration information for each named registry including
         * the registry name and its associated factory instance.
         *
         * @struct RegistryFactoryRegistration
         */
        struct RegistryFactoryRegistration final
        {
            const std::string name;                                ///< Name identifying the registry
            RegistryFactory factory;                               ///< Factory instance for this registry

            /**
             * @brief Creates a registry registration entry.
             *
             * @tparam T Template parameter (unused, maintained for compatibility)
             * @param name Name for the registry
             * @return RegistryRegistration Initialized registration object
             */
            template <typename T = void>
            static RegistryFactoryRegistration create(const std::string& name);

        };
        using Registrator = reg::Registrator<RegistryFactoryRegistration, reg::RegistrationStrategy::UNIQUE>;  ///< Internal registrator type

    public:
        /**
         * @brief Registers a new registry factory by name.
         *
         * Creates and registers a RegistryFactory with the specified name.
         * If a registry with this name already exists, this call is ignored
         * (due to UNIQUE registration strategy).
         *
         * @param name Unique identifier for the registry
         */
        static void Register(const std::string& name);

        /**
        * @brief Retrieves a factory by name.
        *
        * Returns a weak pointer to the RegistryFactory associated with the given name.
        * If no registry exists with this name, returns an empty weak_ptr.
        *
        * @param name Name of the registry to retrieve
        *
        * @note noexcept - guaranteed not to throw exceptions
        */
        static RegistryFactory* GetFactory(const std::string& name) noexcept;

        /**
         * @brief Registers a component type with the factory system.
         *
         * Associates a component type with one or more factory names, enabling
         * runtime creation and management through registered factories.
         *
         * @tparam N Number of factory names (auto-deduced)
         * @param names Factory names for this component (multiple names allowed)
         * @param componentInfo Component metadata for runtime operations
         * @return true if all registrations succeeded, false otherwise
         *
         * @note Component ID must be unique across all components
         */
        template<std::size_t N>
        static bool RegisterComponent(const std::array<std::string_view, N>& names, const RegisterComponentInfo& componentInfo);

    };

}
#include "detail/RegistryRegistrator.ipp"
