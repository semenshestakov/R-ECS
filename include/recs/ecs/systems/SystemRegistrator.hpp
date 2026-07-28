#ifndef SYSTEMS_REGISTRATOR_HPP
#define SYSTEMS_REGISTRATOR_HPP

#include "ecs/systems/SystemsManager.hpp"
#include "reg/Registrator.hpp"

namespace ecs
{
    
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
        /**
         * @brief Internal structure storing registry metadata.
         *
         * Contains registration information for each named registry including
         * the registry name and its associated systemsManager instance.
         */
        struct RegisterSystemInfo final
        {
            const std::string name;                                 ///< Name identifying the registry
            const systemHash_t hash;                                ///< Unique system hash
            baseSystemPtr_t (*makeNew)() = nullptr;                     ///< New constructor function

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
        using Registrator = reg::Registrator<RegisterSystemInfo, reg::RegistrationStrategy::UNIQUE>;  ///< Internal registrator type

    public:
        /**
         * @brief Registers a system class with the registry system
         * @tparam SystemCls The system class to register (must inherit from ISystem)
         * @return Registrator Reference to the registrator instance
         *
         * This function:
         * 1. Creates a registration entry for the system type
         * 2. Retrieves all registry names from SystemCls::GetRegistryNames()
         * 3. Associates the system with each registry name via RegistryRegistrator
         *
         * @note Must be called once per system type, typically during static initialization
         * @warning SystemCls must provide GetRegistryNames() static method
         *
         * @see RegistryRegistrator::RegisterSystem
         */
        template<typename SystemCls>
        static Registrator Register();

        /**
         * @brief Retrieves system registration information by index
         * @param index The registration index to look up
         * @return const RegisterSystemInfo& Constant reference to the registration info
         *
         * Returns the registration information for the system at the specified index.
         * The index is typically obtained during system registration.
         *
         * @pre Index must be valid (0 <= index < GetCount())
         * @throws Assertion failure if index is invalid (in debug builds)
         *
         * @note This is a static method and cannot be const-qualified
         * @see Get(const std::string& name) for lookup by name
         * @see GetIndex(const std::string& name) to obtain index from name
         */
        static const RegisterSystemInfo& Get(std::size_t index);

        /**
         * @brief Retrieves system registration information by index without asserting
         * @param index The registration index to look up
         * @return const RegisterSystemInfo* Pointer to the registration info, or nullptr
         *         if the index does not map to a live registration
         *
         * Unlike Get(), this never dereferences a missing entry, so callers can safely
         * probe an index that may be stale (e.g. desynchronised after a domain reload or
         * sourced from a different module instance of the registry).
         *
         * @see Get(std::size_t index)
         */
        [[nodiscard]] static const RegisterSystemInfo* tryGet(std::size_t index);

        static const void* debugStorage();
        static std::size_t debugSlots();

    };

}

#endif
#include "detail/SystemRegistrator.ipp"
