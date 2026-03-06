#pragma once
#include <map>
#include <vector>
#include "IBaseSystem.hpp"
#include "ecs/utils/SystemUtils.hpp"


namespace ecs
{

    class SystemsManager final
    {
    public:
        SystemsManager();                    ///< @brief Constructs an empty SystemsManager with no registered systems
        ~SystemsManager();                   ///< @brief Destructor that cleans up all managed systems

    private:
        /**
         * @brief Private copy constructor to prevent copying.
         *
         * SystemsManager manages unique ownership of systems and cannot be copied.
         * This constructor is deleted in practice through being private and
         * without implementation.
         *
         * @param other The source SystemsManager to copy from
         */
        SystemsManager(const SystemsManager& other);

        /**
         * @brief Private copy assignment operator to prevent copying.
         *
         * Ensures SystemsManager cannot be copied through assignment, maintaining
         * unique ownership semantics of system instances.
         *
         * @param other The source SystemsManager to copy from
         * @return SystemsManager& Reference to this manager
         */
        SystemsManager& operator=(const SystemsManager& other);

        /**
         * @brief Internal helper method for copy operations.
         *
         * Performs a deep copy of all systems from another SystemsManager.
         * This is used by the move operations to properly transfer ownership
         * and state.
         *
         * @param other The source SystemsManager to copy from
         */
        void copy(const SystemsManager& other);

    public:
        /**
         * @brief Move constructor that transfers ownership of all systems.
         *
         * Creates a new SystemsManager by taking ownership of all systems
         * from another manager, leaving the source manager in an empty state.
         *
         * @param other The source SystemsManager to move from
         */
        SystemsManager(SystemsManager&& other) noexcept;

        /**
         * @brief Move assignment operator that transfers ownership of all systems.
         *
         * Replaces the current manager's contents with systems moved from another
         * manager, properly cleaning up any existing systems first.
         *
         * @param other The source SystemsManager to move from
         * @return SystemsManager& Reference to this manager
         */
        SystemsManager& operator=(SystemsManager&& other) noexcept;

        /**
         * @brief Swaps the contents of two SystemsManager instances.
         *
         * Efficiently exchanges all systems and internal state between two managers
         * without any copying. Provides strong exception guarantee.
         *
         * @param other The SystemsManager to swap with
         */
        void swap(SystemsManager& other) noexcept;

#ifndef DEEP_TEST_ENABLE
    private:
#endif
        using systemPtr_t = std::unique_ptr<IBaseSystem>;           ///< Type alias for system ownership
        using hash_t = std::size_t;                                 ///< Type alias for type hash codes

        std::map<updateTag_t, std::vector<hash_t>> m_updates = {};  ///< Systems organized by update tag
        std::map<hash_t, systemPtr_t> m_systemsMap = {};            ///< Map of type hash to system instance

    public:
        /**
         * @brief Registers a new system type with the manager.
         *
         * Creates an instance of the specified system type and stores it in the manager.
         * The system is automatically organized under its declared UPDATE_TAG for batch
         * updates. Registration fails silently if a system of the same type already exists.
         *
         * @tparam System The system type to register (must derive from IBaseSystem)
         * @return true if the system was successfully registered, false if a system of the same type already exists
         */
        template<typename System>
        bool Register();

        [[maybe_unused]] bool Init(const InitState &state);

        /**
         * @brief Updates all systems or systems with a specific update tag.
         *
         * Executes the Update method on all registered systems, optionally filtered by
         * update tag. When a tag is provided, only systems with that tag are updated;
         * otherwise, all systems are updated. The update order respects the registration
         * order within each tag group.
         *
         * @param registry Reference to the main Registry for system operations
         * @param updateTag Optional tag to filter which systems to update
         */
        void Update(Registry& registry, std::optional<updateTag_t> updateTag = std::nullopt);

        [[nodiscard]] std::size_t size() const;

        template<typename System>
        [[nodiscard]] System* get();

        friend class Registry;
    };


    template<typename System>
    bool SystemsManager::Register()
    {
        const hash_t hash = typeid(System).hash_code();
        if (m_systemsMap.contains(hash))
            return false;

        m_systemsMap[hash] = std::make_unique<System>();
        m_updates[System::UPDATE_TAG].push_back(hash);
        return true;
    }

    template<typename System>
    System* SystemsManager::get()
    {
        const auto it = m_systemsMap.find(typeid(System).hash_code());
        if (it == m_systemsMap.end())
            return nullptr;

        return dynamic_cast<System*>(it->second.get());
    }

}

