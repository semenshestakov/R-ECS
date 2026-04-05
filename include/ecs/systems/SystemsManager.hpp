#ifndef SYSTEM_MANAGER_HPP
#define SYSTEM_MANAGER_HPP

#include <unordered_map>
#include <cassert>
#include "common_recs/utils/ClassUtils.hpp"
#include "IBaseSystem.hpp"
#include "SystemsSchedule.hpp"
#include "ecs/utils/SystemUtils.hpp"


namespace ecs
{

    class SystemsManager final
    {
    public:
        SystemsManager();                    ///< @brief Constructs an empty SystemsManager with no registered systems
        ~SystemsManager();                   ///< @brief Destructor that cleans up all managed systems

    DEEP_TEST_PRIVATE_ACCESS:
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

    DEEP_TEST_PRIVATE_ACCESS:
        SystemsSchedule m_schedule;                                                 ///< Schedule for update systems
        using systemPtr_t = std::unique_ptr<IBaseSystem>;                           ///< Type alias for system ownership
        std::unordered_map<systemHash_t, systemPtr_t> m_systemsMap = {};            ///< Map of type hash to system instance

        /**
         * @brief Initializes all registered systems with the given state
         *
         * Calls the Init method on every registered system, passing the provided
         * initialization state. The order of initialization respects system
         * registration order.
         *
         * @param state The initialization state to pass to systems
         * @return true if all systems initialized successfully, false otherwise
         *
         * @note Systems must implement the Init method as per IBaseSystem interface
         */
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
         */
        void Update(Registry& registry);

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
        template<typename System> bool Register();

        /**
         * @brief Returns the number of registered systems
         * @return std::size_t Total count of systems currently managed
         */
        [[nodiscard]] std::size_t size() const;

        /**
         * @brief Retrieves a registered system by its type
         *
         * Provides access to a specific system instance through its type.
         * Returns nullptr if no system of the requested type is registered.
         *
         * @tparam SystemCls The type of system to retrieve
         * @return Pointer to the system instance, or nullptr if not found
         *
         * @note The returned pointer remains valid until the system is unregistered
         *       or the manager is destroyed
         */
        template<typename SystemCls> [[nodiscard]] SystemCls& Get();
        template<typename SystemCls> [[nodiscard]] const SystemCls& Get() const;

        template<typename SystemCls> [[nodiscard]] SystemCls* TryGet();
        template<typename SystemCls> [[nodiscard]] const SystemCls* TryGet() const;
    };

} // namespace ecs
#endif
#include "detail/SystemsManager.ipp"
