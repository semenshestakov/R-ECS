#ifndef REGISTRY_REGISTRATOR_HPP
#define REGISTRY_REGISTRATOR_HPP

#include "ecs/systems/SystemsManager.hpp"
#include "reg/Registrator.hpp"

namespace ecs
{

    /**
     * @brief Internal structure storing registry metadata.
     *
     * Contains registration information for each named registry including
     * the registry name and its associated systemsManager instance.
     */
    struct RegisterSystemInfo final
    {
        const std::string name;                                 ///< Name identifying the registry
        SystemsManager systemManager;                           ///< SystemsManager instance for this registry

        /**
         * @brief Creates a registry registration entry.
         *
         * @tparam T Template parameter (unused, maintained for compatibility)
         * @param name Name for the registry
         * @return RegistryInfo Initialized registration object
         */
        template <typename T = void>
        static RegisterSystemInfo Create(const std::string& name);
    };

    /**
     * @brief Manages registration and retrieval of SystemRegistrator instances.
     *
     * The SystemRegistrator provides a central registry for creating and accessing
     * objects by name. It uses a unique registration strategy
     * ensuring that each registry name is registered only once throughout the
     * application lifetime.
     *
     * @note Uses RegistrationStrategy::UNIQUE - each name can have only one systemsManager
     * @note Returns std::weak_ptr to factories to avoid ownership cycles
     * @see reg::Registrator
     */
    class SystemRegistrator final
    {
    DEEP_TEST_PRIVATE_ACCESS:
        using Registrator = reg::Registrator<RegisterSystemInfo, reg::RegistrationStrategy::UNIQUE>;  ///< Internal registrator type

    public:
        /**
         * @brief Registers a new registry systemsManager by name.
         *
         * Creates and registers a SystemsManager with the specified name.
         * If a registry with this name already exists, this call is ignored
         * (due to UNIQUE registration strategy).
         *
         * @param name Unique identifier for the registry
         */
        static void RegisterRegistry(const std::string& name);

        static SystemsManager* GetSystemsManager(const std::string& name) noexcept;

        /**
         * @brief Registers a system type with the systemsManager system.
         *
         * @tparam N Number of systemsManager names (auto-deduced)
         * @param names Factory names for this system (multiple names allowed)
         * @return true if all registrations succeeded, false otherwise
         */
        template<typename System, std::size_t N>
        static bool Register(const std::array<std::string_view, N>& names);

    };

}

#include "detail/SystemRegistrator.ipp"
#endif
