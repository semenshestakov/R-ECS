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
    class RegistryRegistrator
    {
        /**
         * @brief Internal structure storing registry metadata.
         *
         * Contains registration information for each named registry including
         * the registry name and its associated factory instance.
         *
         * @struct RegistryRegistration
         */
        struct RegistryRegistration
        {
            const std::string name;                                ///< Name identifying the registry
            std::shared_ptr<RegistryFactory> factory = nullptr;    ///< Factory instance for this registry

            /**
             * @brief Creates a registry registration entry.
             *
             * @tparam T Template parameter (unused, maintained for compatibility)
             * @param name Name for the registry
             * @return RegistryRegistration Initialized registration object
             */
            template <typename T = void>
            static RegistryRegistration create(const std::string& name);

        };
        using _Registrator = reg::Registrator<RegistryRegistration, reg::RegistrationStrategy::UNIQUE>;  ///< Internal registrator type

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
        * @return std::weak_ptr<RegistryFactory> Weak pointer to the factory,
        *         or empty weak_ptr if not found
        *
        * @note Returns weak_ptr to avoid ownership cycles - caller should lock()
        *       to obtain a usable shared_ptr
        * @note noexcept - guaranteed not to throw exceptions
        */
        static std::weak_ptr<RegistryFactory> getFactory(const std::string& name) noexcept;

    };

}
#include "detail/RegistryRegistrator.ipp"
