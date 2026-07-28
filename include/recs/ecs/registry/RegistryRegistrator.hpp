/**
 * @file RegistryRegistrator.hpp
 * @brief Manages registration of ECS registries and their associated systems
 *
 * This file provides a centralized registry for managing named ECS registries
 * and the systems that belong to each registry. It enables dynamic lookup
 * and association between registry names and system indices.
 */

#ifndef REGISTRY_REGISTRATOR_HPP
#define REGISTRY_REGISTRATOR_HPP
#include <vector>
#include "reg/Registrator.hpp"


namespace ecs
{


    /**
     * @brief Centralized manager for ECS registry registration and system associations
     *
     * The RegistryRegistrator class provides a singleton-like mechanism to register
     * named registries and associate system indices with them. It uses a unique
     * registration strategy to ensure each registry name is registered only once.
     *
     * This is typically used during system initialization to associate systems
     * with specific registry instances identified by name.
     *
     * @note Implements thread-safe registration using the underlying Registrator
     * @see reg::Registrator
     * @see RegistryInfo
     *
     * @par Example:
     * @code
     * // Register a registry
     * RegistryRegistrator::Register("game_world");
     *
     * // Associate a system with the registry
     * RegistryRegistrator::RegisterSystem("game_world", systemIndex);
     *
     * // Retrieve registry info
     * const auto& info = RegistryRegistrator::Get("game_world");
     * for (auto idx : info.systemRegIndexes) {
     *     // Process associated systems
     * }
     * @endcode
     */
    class RegistryRegistrator final
    {
    DEEP_TEST_PRIVATE_ACCESS:

        /**
         * @brief Internal structure holding registry metadata
         *
         * Stores the registry name and a list of system registration indices
         * that are associated with this registry.
         */
        struct RegistryInfo
        {
            const std::string name;                                 ///< Unique registry name identifier
            std::vector<std::size_t> systemRegIndexes;              ///< Indices of systems registered to this registry

            /**
             * @brief Factory method to create RegistryInfo with a given name
             * @tparam T Unused template parameter (for compatibility)
             * @param name The registry name
             * @return RegistryInfo instance with empty system list
             */
            template <typename T = void>
            static RegistryInfo Create(const std::string& name);
        };

        /// @brief Underlying registrator using unique strategy (no duplicate names)
        using Registrator = reg::Registrator<RegistryInfo, reg::RegistrationStrategy::UNIQUE>;

    public:
        /**
         * @brief Registers a new registry with the given name
         * @param name Unique name identifier for the registry
         *
         * Creates a new RegistryInfo entry with the specified name.
         * If a registry with the same name already exists, registration fails
         * due to UNIQUE strategy.
         *
         * @note Must be called before RegisterSystem for the same name
         * @warning Names are case-sensitive
         */
        static void Register(const std::string& name);

        /**
         * @brief Associates a system index with a registered registry
         * @param name Name of the registry to associate with
         * @param systemIndex Index of the system being registered
         *
         * Adds the system index to the systemRegIndexes vector of the
         * specified registry. The registry must have been previously
         * registered with Register().
         *
         * @pre Registry with given name must exist
         * @note Multiple systems can be associated with the same registry
         */
        static void RegisterSystem(const std::string& name, std::size_t systemIndex);

        /**
         * @brief Retrieves registry information by name
         * @param name Name of the registry to lookup
         * @return const RegistryInfo& Constant reference to the registry info
         *
         * Returns the RegistryInfo structure containing the registry name
         * and all associated system indices.
         *
         * @pre Registry with given name must exist
         * @throws std::out_of_range if registry name not found
         */
        static const RegistryInfo& Get(const std::string& name);

        /**
         * @brief Checks whether a registry with the given name exists.
         * @param name Name of the registry to look up
         * @return true if a registry with the specified name was registered, false otherwise
         *
         * Unlike Get(), this never asserts or dereferences a missing entry, so callers can
         * safely probe optional archetypes (e.g. a puzzle that contributes no systems).
         */
        [[nodiscard]] static bool contains(const std::string& name);
    };

} // namespace ecs
#endif
#include "detail/RegistryRegistrator.ipp"
