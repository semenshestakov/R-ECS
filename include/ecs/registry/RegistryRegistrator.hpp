#pragma once
#include "../components/ComponentsManager.hpp"
#include "ecs/systems/SystemsManager.hpp"
#include "reg/Registrator.hpp"

namespace ecs
{

    /**
     * @brief Manages registration and retrieval of ComponentsManager instances.
     *
     * The RegistryRegistrator provides a central registry for creating and accessing
     * ComponentsManager objects by name. It uses a unique registration strategy
     * ensuring that each registry name is registered only once throughout the
     * application lifetime.
     *
     * @note Uses RegistrationStrategy::UNIQUE - each name can have only one componentsManager
     * @note Returns std::weak_ptr to factories to avoid ownership cycles
     * @see reg::Registrator
     * @see ComponentsManager
     */
    class RegistryRegistrator final
    {
#ifdef DEEP_TEST_ENABLE
    public:
#endif
        /**
         * @brief Internal structure storing registry metadata.
         *
         * Contains registration information for each named registry including
         * the registry name and its associated componentsManager instance.
         *
         * @struct RegistryInfo
         */
        struct RegistryInfo final
        {
            const std::string name;                                 ///< Name identifying the registry
            SystemsManager systemManager;                            ///< SystemsManager instance for this registry
            ComponentsManager componentsManager;                    ///< ComponentsManager instance for this registry

            /**
             * @brief Creates a registry registration entry.
             *
             * @tparam T Template parameter (unused, maintained for compatibility)
             * @param name Name for the registry
             * @return RegistryInfo Initialized registration object
             */
            template <typename T = void>
            static RegistryInfo Create(const std::string& name);

        };
        using Registrator = reg::Registrator<RegistryInfo, reg::RegistrationStrategy::UNIQUE>;  ///< Internal registrator type

    public:
        /**
         * @brief Registers a new registry componentsManager by name.
         *
         * Creates and registers a ComponentsManager with the specified name.
         * If a registry with this name already exists, this call is ignored
         * (due to UNIQUE registration strategy).
         *
         * @param name Unique identifier for the registry
         */
        static void Register(const std::string& name);

        /**
        * @brief Retrieves a componentsManager by name.
        *
        * Returns a weak pointer to the ComponentsManager associated with the given name.
        * If no registry exists with this name, returns an empty weak_ptr.
        *
        * @param name Name of the registry to retrieve
        *
        * @note noexcept - guaranteed not to throw exceptions
        */
        static ComponentsManager* GetComponentsManager(const std::string& name) noexcept;

        static SystemsManager* GetSystemsManager(const std::string& name) noexcept;

        /**
         * @brief Registers a component type with the componentsManager system.
         *
         * Associates a component type with one or more componentsManager names, enabling
         * runtime creation and management through registered factories.
         *
         * @tparam N Number of componentsManager names (auto-deduced)
         * @param names Factory names for this component (multiple names allowed)
         * @param componentInfo Component metadata for runtime operations
         * @return true if all registrations succeeded, false otherwise
         *
         * @note Component ID must be unique across all components
         */
        template<std::size_t N>
        static bool RegisterComponent(const std::array<std::string_view, N>& names, const RegisterComponentInfo& componentInfo);

        /**
         * @brief Registers a system type with the componentsManager system.
         *
         * @tparam N Number of componentsManager names (auto-deduced)
         * @param names Factory names for this system (multiple names allowed)
         * @return true if all registrations succeeded, false otherwise
         */
        template<typename System, std::size_t N>
        static bool RegisterSystem(const std::array<std::string_view, N>& names);

    };

}

#include "detail/RegistryRegistrator.ipp"
