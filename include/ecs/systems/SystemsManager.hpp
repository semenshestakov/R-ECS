#ifndef SYSTEM_MANAGER_HPP
#define SYSTEM_MANAGER_HPP

#include <span>
#include <unordered_map>
#include "IBaseSystem.hpp"
#include "SystemsSchedule.hpp"
#include "Utils.hpp"
#include "common_recs/utils/ClassUtils.hpp"


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
        std::unordered_map<systemHash_t, baseSystemPtr_t> m_systemsMap = {};        ///< Map of type hash to system instance

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
         * @brief Subscribes all registered systems to the event system with stage-based priority.
         *
         * Iterates through the internal system schedule and subscribes each system
         * to the provided EventSystem using a computed priority value.
         *
         * Priority is assigned based on system execution stage:
         * - The first stage receives the highest priority (MAX_PRIORITY)
         * - Each subsequent stage receives a decremented priority value
         *
         * This ensures deterministic ordering of system callbacks during event dispatch.
         *
         * @param state Subscription state containing the EventSystem reference and base configuration.
         *               A local copy is created for each system with an assigned priority.
         *
         * @return true Always returns true after successful subscription of all systems.
         */
        [[maybe_unused]] bool Subscribe(const SubscribeState& state);

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
         * @brief Registers a system by its registration index
         * @param systemRegIndex The registration index obtained from SystemRegistrator
         * @return true if the system was successfully registered, false if already exists
         *
         * This method creates a system instance using the factory function stored in
         * SystemRegistrator and adds it to the manager's internal storage.
         *
         * @pre systemRegIndex must be valid (obtained from SystemRegistrator::Register)
         * @note The system is automatically added to the update schedule
         * @see SystemRegistrator::Get
         */
        bool Register(std::size_t systemRegIndex);

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

        /**
         * @brief Attempts to retrieve a registered system by type (non-const version)
         * @tparam SystemCls The system type to retrieve
         * @return SystemCls* Pointer to the system instance, or nullptr if not found
         *
         * Safe non-throwing version of Get(). Returns nullptr instead of throwing
         * an exception when the system is not registered.
         */
        template<typename SystemCls> [[nodiscard]] SystemCls* TryGet();
        template<typename SystemCls> [[nodiscard]] const SystemCls* TryGet() const;


        /**
         * @brief Creates a SystemsManager instance with pre-registered systems
         * @param systemRegIndexes Span of system registration indices to include
         * @return SystemsManager A new manager instance containing the specified systems
         *
         * Factory method that constructs a SystemsManager and registers all systems
         * whose indices are provided in the span. This is more efficient than creating
         * an empty manager and registering systems individually.
         *
         * @note The systems are registered in the order they appear in the span
         * @warning The span must contain valid registration indices
         */
        static SystemsManager Create(std::span<const std::size_t> systemRegIndexes);
    };

} // namespace ecs
#endif
#include "detail/SystemsManager.ipp"
