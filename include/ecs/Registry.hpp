#pragma once
#include "collections/Context.hpp"
#include "common_recs/utils/ClassUtils.hpp"
#include "entities/EntitiesManager.hpp"
#include "systems/EventSystem.hpp"
#include "systems/SystemsManager.hpp"


namespace ecs
{

    /**
     * @brief Central coordination hub for the ECS framework.
     *
     * Registry serves as the main entry point for the ECS system, aggregating
     * and coordinating all core components:
     * - Entity management (creation, destruction, component access)
     * - Event system (communication between systems)
     * - System management (execution order, lifecycle)
     * - Context storage (shared data across systems)
     *
     * The Registry provides a unified interface for interacting with the ECS,
     * acting as a facade that delegates operations to its internal components.
     * It manages the initialization and update cycle of all registered systems
     * while maintaining proper execution order based on dependencies.
     *
     * @note Registry is non-copyable but movable, designed for single ownership.
     *
     * @example
     * // Create registry with system manager
     * SystemsManager sysManager;
     * Registry registry(std::move(sysManager));
     *
     * // Initialize all systems
     * registry.Init();
     *
     * // Main game loop
     * while (running) {
     *     ...
     *     registry.Update();  // Updates all systems in correct order
     *     ...
     * }
     */
    class Registry final
    {
    public:
        /**
         * @brief Constructs registry with a systems manager.
         * @param systemManager Systems manager containing registered systems
         *
         * @note Takes ownership of the systems manager via move semantics.
         */
        explicit Registry(SystemsManager systemManager);

        Registry();                                                     ///< Default construction
        ~Registry() = default;                                          ///< Default destructor
        Registry(Registry&&) noexcept = default;                        ///< Move constructible
        Registry& operator=(Registry&&) noexcept = default;             ///< Move assignable
        Registry(const Registry&) = delete;                             ///< Non-copyable
        Registry& operator=(const Registry&) = delete;                  ///< Non-copyable

    DEEP_TEST_PRIVATE_ACCESS:
        EntitiesManager m_entitiesManager;          ///< Underlying entity storage
        EventSystem m_eventSystem;                  ///< Local Event System
        SystemsManager m_systemManager;             ///< Underlying entity storage
        collections::Context m_context;             ///< Context (Data storage)

    public:
        /**
         * @brief Gets mutable reference to entity manager.
         * @return Reference to EntitiesManager for entity operations
         */
        [[nodiscard]] EntitiesManager& Entities() { return m_entitiesManager; }

        /**
         * @brief Gets const reference to entity manager.
         * @return Const reference to EntitiesManager for read-only entity operations
         */
        [[nodiscard]] const EntitiesManager& Entities() const { return m_entitiesManager; }

        /**
         * @brief Gets mutable reference to event system.
         * @return Reference to EventSystem for publishing/subscribing to events
         */
        [[nodiscard]] EventSystem& Events() { return m_eventSystem; }

        /**
         * @brief Gets const reference to event system.
         * @return Const reference to EventSystem for read-only event operations
         */
        [[nodiscard]] const EventSystem& Events() const { return m_eventSystem; }

        /**
         * @brief Gets mutable reference to systems manager.
         * @return Reference to SystemsManager for system registration and control
         */
        [[nodiscard]] SystemsManager& Systems() { return m_systemManager; }

        /**
         * @brief Gets const reference to systems manager.
         * @return Const reference to SystemsManager for read-only system queries
         */
        [[nodiscard]] const SystemsManager& Systems() const { return m_systemManager; }

        /**
         * @brief Gets mutable reference to context storage.
         * @return Reference to Context for accessing shared data
         */
        [[nodiscard]] collections::Context& ctx() { return m_context; }

        /**
         * @brief Gets const reference to context storage.
         * @return Const reference to Context for read-only shared data access
         */
        [[nodiscard]] const collections::Context& ctx() const { return m_context; }

        /**
         * @brief Initializes all registered systems.
         *
         * Calls the Init() method on all systems in the appropriate order,
         * passing the provided arguments. Systems are initialized according
         * to their dependency graph to ensure prerequisites are ready.
         *
         * @param args Optional pointer to initialization arguments
         * @return true if all systems initialized successfully, false otherwise
         *
         * @note Should be called once after all systems are registered and
         *       before the first Update() call.
         */
        [[maybe_unused]] bool Init(void* args = nullptr);

        /**
         * @brief Updates all registered systems.
         *
         * Executes the Update() method on all systems in dependency order.
         * Should be called once per frame or update cycle. Systems are
         * processed according to their topological order based on declared
         * dependencies.
         *
         * @note Assumes Init() has been called successfully before first Update().
         * @note Events queued during Update() are processed after all systems
         *       have executed, or immediately based on event system configuration.
         */
        void Update();

        /**
         * @brief Creates a fully configured registry by name
         * @param name The registry name used to look up system configurations
         * @return Registry A new registry instance with pre-registered systems
         *
         * Factory method that creates a Registry with systems automatically registered
         * based on the named configuration. This is the preferred way to create a
         * registry when using declarative system registration.
         *
         * The method:
         * 1. Looks up registry information by name from RegistryRegistrator
         * 2. Creates a SystemsManager with all systems registered to this registry
         * 3. Returns a Registry instance owning that SystemsManager
         *
         * @pre Registry with given name must have been registered with RegistryRegistrator
         * @throws assert if registry name not found
         *
         * @par Example:
         * @code
         * auto registry = Registry::Create("game_world");
         * registry.Init();
         * while (running) {
         *     registry.Update();
         * }
         * @endcode
         *
         * @see RegistryRegistrator
         * @see SystemsManager::Create
         */
        static Registry Create(const std::string& name);
    };

} // namespace ecs
