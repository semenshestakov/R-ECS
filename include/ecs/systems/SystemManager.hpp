#pragma once
#include <map>
#include <vector>
#include "SystemUtils.hpp"


namespace ecs
{

    class SystemManager final
    {
    public:
        SystemManager();                    ///< @brief Constructs an empty SystemManager with no registered systems
        ~SystemManager();                   ///< @brief Destructor that cleans up all managed systems

    private:
        /**
         * @brief Private copy constructor to prevent copying.
         *
         * SystemManager manages unique ownership of systems and cannot be copied.
         * This constructor is deleted in practice through being private and
         * without implementation.
         *
         * @param other The source SystemManager to copy from
         */
        SystemManager(const SystemManager& other);

        /**
         * @brief Private copy assignment operator to prevent copying.
         *
         * Ensures SystemManager cannot be copied through assignment, maintaining
         * unique ownership semantics of system instances.
         *
         * @param other The source SystemManager to copy from
         * @return SystemManager& Reference to this manager
         */
        SystemManager& operator=(const SystemManager& other);

        /**
         * @brief Internal helper method for copy operations.
         *
         * Performs a deep copy of all systems from another SystemManager.
         * This is used by the move operations to properly transfer ownership
         * and state.
         *
         * @param other The source SystemManager to copy from
         */
        void copy(const SystemManager& other);

    public:
        /**
         * @brief Move constructor that transfers ownership of all systems.
         *
         * Creates a new SystemManager by taking ownership of all systems
         * from another manager, leaving the source manager in an empty state.
         *
         * @param other The source SystemManager to move from
         */
        SystemManager(SystemManager&& other) noexcept;

        /**
         * @brief Move assignment operator that transfers ownership of all systems.
         *
         * Replaces the current manager's contents with systems moved from another
         * manager, properly cleaning up any existing systems first.
         *
         * @param other The source SystemManager to move from
         * @return SystemManager& Reference to this manager
         */
        SystemManager& operator=(SystemManager&& other) noexcept;

        /**
         * @brief Swaps the contents of two SystemManager instances.
         *
         * Efficiently exchanges all systems and internal state between two managers
         * without any copying. Provides strong exception guarantee.
         *
         * @param other The SystemManager to swap with
         */
        void swap(SystemManager& other) noexcept;

    private:
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
         * @tparam SystemT The system type to register (must derive from IBaseSystem)
         * @return true if the system was successfully registered, false if a system of the same type already exists
         */
        template<typename SystemT>
        bool Register();

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

        friend class Registry;
    };


    template<typename SystemT>
    bool SystemManager::Register()
    {
        const hash_t hash = typeid(SystemT).hash_code();
        if (m_systemsMap.contains(hash))
            return false;

        m_systemsMap[hash] = std::make_unique<SystemT>();
        m_updates[SystemT::UPDATE_TAG].push_back(hash);
        return true;
    }

}

