#pragma once

#include "common_recs/utils/ClassUtils.hpp"
#include "components/Components.hpp"
#include "components/ComponentsManager.hpp"
#include "entities/EntitiesManager.hpp"
#include "entities/ranges/EntitiesViews.hpp"
#include "event/EventSystem.hpp"
#include "systems/SystemsManager.hpp"


namespace ecs
{

    /**
     * @brief Central coordinator and primary interface for the ECS (Entity-Component-System) architecture.
     *
     * The Registry class serves as the main entry point for all ECS operations, managing the complete
     * lifecycle of entities, components, and systems. It acts as a facade that coordinates between
     * three specialized managers:
     * - EntitiesManager: Handles entity storage, iteration, and lifetime
     * - ComponentsManager: Manages component factories and type registration
     * - SystemsManager: Controls system registration and update execution
     *
     * Key responsibilities:
     * - Entity creation and existence checking
     * - Component access and manipulation through entity handles
     * - System updates with configurable update phases
     * - Range-based views for efficient querying of entities with specific components
     *
     * The Registry is designed with clear ownership semantics:
     * - Move constructible and assignable for efficient transfer
     * - Non-copyable to prevent duplication of unique resources
     * - Default constructible for flexibility in initialization
     *
     * Thread Safety: Not thread-safe. External synchronization required for multi-threaded access.
     *
     * Example usage:
     * @code
     * // Create and initialize registry
     * auto entitiesManager = EntitiesManager::Create<MyEntityManager>();
     * ComponentsManager componentsManager; ... // Register Transform, Velocity components
     * Registry registry(std::move(entitiesManager), componentsManager);
     *
     * // Create entities
     * auto& entity1 = registry.Create();
     * auto& entity2 = registry.Create(42); // Specific ID
     *
     * // Query entities with specific components
     * for (auto [transform, velocity] : registry.view<Transform, Velocity>()) {
     *     transform.position.x += velocity.dx;
     * }
     *
     * // Update systems
     * registry.Update(updateTag_t::Physics);
     * @endcode
     *
     * @see EntitiesManager for entity storage details
     * @see ComponentsManager for component componentsManager management
     * @see SystemsManager for system execution control
     */
    class Registry final
    {
    public:
        /// Constructor - requires componentsManager for initialization
        explicit Registry(EntitiesManager&& entitiesManager, const ComponentsManager& componentsManager);
        explicit Registry(EntitiesManager&& entitiesManager, const ComponentsManager& componentsManager, const SystemsManager& systemManager);

        Registry() = default;                                           ///< Default construction
        ~Registry() = default;                                          ///< Default destructor
        Registry(Registry&&) noexcept = default;                        ///< Move constructible
        Registry& operator=(Registry&&) noexcept = default;             ///< Move assignable
        Registry(const Registry&) = delete;                             ///< Non-copyable
        Registry& operator=(const Registry&) = delete;                  ///< Non-copyable

    DEEP_TEST_PRIVATE_ACCESS:
        EntitiesManager m_entitiesManager;          ///< Underlying entity storage
        ComponentsManager m_componentsManager;      ///< Factory instance for component operations
        EventSystem m_eventSystem;                  ///< Local Event System
        SystemsManager m_systemManager;             ///< Underlying entity storage

    public:
        /// Find components for entity. Returns nullptr if not found.
        [[nodiscard]] Components* get(entityId_t entityId) const;

        /// Find system. Returns nullptr if not found.
        template<typename System>
        [[nodiscard]] System* getSystem();

        /// Check if entity exists in registry
        [[nodiscard]] bool contains(entityId_t entityId) const;

        /// Get count entities
        [[nodiscard]] std::size_t size() const;

        /// Get components for entity. Asserts/throws if entity not found.
        [[nodiscard]] Components& mustGet(entityId_t entityId) const;

        [[maybe_unused]] bool Init(void* args = nullptr);
        /**
         * Create components for given entity ID.
         *
         * @param entityId Entity identifier
         * @return Reference to created components
         * @throws error::InvalidEntityId if entity already exists
         */
        [[maybe_unused]] Components& Create(entityId_t entityId);

        /**
         * Create components with auto-generated entity ID.
         *
         * @return Reference to created Entity{.id, .components}
         * @requires EntitiesManager must have generateId() method
         */
        [[maybe_unused]] Entity Create();

        /**
         * @brief Updates all systems in the registry.
         *
         * @param updateTag Optional tag to control which systems should update.
         *                  If nullopt (default), all systems are updated.
         *                  If provided, only systems matching the tag are updated.
         *
         * @note This method delegates to SystemsManager::Update().
         * @note Systems are updated in the order they were registered.
         * @note The registry itself is passed to systems for component access.
         *
         * Example usage:
         * @code
         * registry.Update();                    // Update all systems
         * registry.Update(updateTag_t::Physics); // Update only physics systems
         * @endcode
         *
         * @see SystemsManager::Update() for detailed update semantics
         */
        void Update(std::optional<updateTag_t> updateTag = std::nullopt);

        /**
         * @brief Creates a view over entities that have all specified component types.
         *
         * @tparam ComponentCls... Variadic list of component types that entities must possess.
         * @return ranges::view::ComponentsViews<ComponentCls...> A range view for iteration.
         *
         * @note The view is lazy - it doesn't create a new container, just provides iteration.
         * @note Entities are iterated in their natural storage order.
         *
         * Example usage:
         * @code
         * for (auto [pos, vel] : registry.view<Position, Velocity>())
         *     pos.x += vel.dx;  // Direct access to component references
         *
         * for (auto& components : registry.view())
         *     components.mustGet<Position>().x += components.mustGet<Velocity>().dx // if present
         *
         * @endcode
         *
         * @see ComponentsViews for iterator implementation details
         */
        template<typename... ComponentCls>
        ranges::view::ComponentsViews<ComponentCls...> view();

        /**
         * @brief Immediately triggers an event with the given data
         *
         * @tparam Event The type of event to trigger
         * @param event The event data to pass to all registered handlers
         *
         * This method:
         * 1. Generates a key from the event type using getEventKey<Event>()
         * 2. Retrieves the corresponding event from the EventSystem
         * 3. Triggers the event immediately with the provided data
         *
         * @note If no event is registered for this type, the call is silently ignored
         * @note The event is processed synchronously - all handlers are called before this method returns
         *
         * @par Example:
         * @code
         * Registry registry;
         *
         * PlayerDiedEvent event{playerId, deathCause};
         * registry.onEvent(event);
         *
         * registry.onEvent(PlayerDiedEvent{42, "explosion"});
         * @endcode
         */
        template<typename Event>
        void onEvent(const Event& event);
    };

    template<typename System>
    System* Registry::getSystem()
    {
        return m_systemManager.get<System>();
    }

    template<typename... ComponentCls>
    ranges::view::ComponentsViews<ComponentCls...> Registry::view()
    {
        return {m_entitiesManager.begin(), m_entitiesManager};
    }

    template<typename Event>
    void Registry::onEvent(const Event& event)
    {
        m_eventSystem.on<Registry&, const Event&>(getEventKey<Event>(), *this, event);
    }

} // namespace ecs
