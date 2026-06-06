#ifndef I_SYSTEM_HPP
#define I_SYSTEM_HPP

/**
 * @file ISystem.hpp
 * @brief Base interface template for all ECS systems with automatic event registration
 *
 * This header defines the core system interface that all concrete systems must inherit from.
 * It provides automatic registration capabilities and event handling infrastructure.
 */

#include <array>
#include "common_recs/utils/ClassUtils.hpp"
#include "event/EventSystem.hpp"
#include "systems/IBaseSystem.hpp"
#include "systems/SystemRegistrator.hpp"


namespace ecs
{

    /**
     * @brief Template base class for all ECS systems
     *
     * @tparam SystemCls The concrete system class (CRTP pattern)
     *
     * This class serves as the foundation for all systems in the ECS architecture.
     * It provides:
     * - Automatic system registration with the Registry
     * - Event registration and management capabilities
     * - Default implementations of IBaseSystem interface methods
     * - Type-safe event listener creation
     *
     * @note Uses CRTP (Curiously Recurring Template Pattern) for static polymorphism
     * @note Automatically registers the system type during static initialization
     *
     * @par Example:
     * @code
     * class MySystem : public ISystem<MySystem> {
     * public:
     *     void Update(Registry& registry, const UpdateState& state) override
     *     {
     *         // System logic here
     *     }
     *
     *     void onPlayerDied(Registry& registry, const PlayerDiedEvent& event);
     *     ECS_EVENT(onPlayerDied, PlayerDiedEvent)
     * };
     * @endcode
    */
    template <typename SystemCls>
    struct ISystem : IBaseSystem
    {
    DEEP_TEST_PROTECTED_ACCESS:
        friend class SystemsManager;
        using Super = ISystem<SystemCls>;                   ///< @brief Alias for the base class (ISystem<SystemCls>)
        using SelfSystemCls = SystemCls;                    ///< @brief Alias for the concrete system class

                                                            ///  @brief Event listener type alias for a specific event
                                                            ///  @tparam Event The event type to listen for
        template <class Event> using EventListener = event::Listener<event::callbackId_t, Registry&, const Event&>;

    public:
        /**
         * @brief Default constructor
         */
        ISystem() : IBaseSystem() { }

        /**
         * @brief Creates a new instance of the concrete system
         * @return Pointer to newly allocated system instance
         *
         * @note Implements IBaseSystem interface
         */
        [[nodiscard]] IBaseSystem* New() const override;

        /**
         * @brief Gets the type name of the concrete system
         * @return string_view containing the demangled type name
         *
         * @note Implements IBaseSystem interface
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Initializes the system with given state
         * @param state Initialization parameters
         *
         * @note Implements IBaseSystem interface
         */
        void Init(const InitState& state) override;

        /**
         * @brief Subscribes the system's event registrations to the provided EventSystem.
         *
         * Executes all previously registered event-binding functions stored in the system
         * and attaches them to the given EventSystem instance. Each function typically
         * binds the system's handlers to specific events.
         *
         * After successful subscription, the internal list of registration functions
         * is cleared to prevent duplicate subscriptions.
         *
         * @tparam SystemCls Concrete system class type used for CRTP-based system design.
         *
         * @param state Subscription state containing a reference to the EventSystem
         *              and additional subscription parameters such as priority.
         *
         * @note This method is typically called once during system initialization.
         * @note After execution, m_registerEventFunctions is cleared and cannot be reused
         *       unless re-populated explicitly.
         */
        void Subscribe(const SubscribeState& state) override;

    DEEP_TEST_PROTECTED_ACCESS:
        /**
         * @brief Registers an event handler for a specific event type
         *
         * @tparam Event The event type to handle
         * @param method Pointer to member function that handles the event
         * @return std::unique_ptr<EventListener<Event>> Listener instance managing the subscription
         *
         * Creates and registers an event listener for the specified event type.
         * The listener will automatically subscribe to the event when the event system
         * becomes available during system initialization.
         *
         * @note The returned unique_ptr must be stored as a member to keep the listener alive
         * @see ECS_EVENT macro for convenient member declaration
         */
        template <class Event>
        auto RegisterEvent(void (SystemCls::*method)(Registry&, const Event&)) -> std::unique_ptr<EventListener<Event>>;

    DEEP_TEST_PRIVATE_ACCESS:
        /**
         * @brief Collection of event registration functions
         *
         * Stores lambdas that will register event listeners when the event system
         * becomes available during system initialization.
         */
        std::vector<std::function<void(EventSystem&, event::priority_t)>> m_registerEventFunctions;

        /**
         * @brief Creates a array of system hashes for the specified system types.
         *
         * This function generates a `std::array` containing the unique hash values of the provided
         * system types. It is primarily used for dependency management, allowing systems to
         * declare their dependencies on other systems in a type-safe.
         *
         * @tparam SystemsArgs The system types to generate hashes for. Each type should be
         *                     a valid system class that has a corresponding `getSystemHash`
         *                     specialization.
         *
         * @return std::array<systemHash_t, sizeof...(SystemsArgs)> A constexpr array containing
         *         the hash values of all specified system types, in the same order as the
         *         template arguments.
         *
         * @note This function is `constexpr`, meaning the array is fully evaluated at compile
         *       time, resulting in zero runtime overhead.
         *
         * @warning The `getSystemHash<SystemType>()` function must be `constexpr` for each
         *          system type to enable compile-time evaluation. If any of the hashes cannot
         *          be computed at compile time, the function will fail to be `constexpr`.
         *
         * @example
         * @code
         * // Generate hashes for three systems
         * constexpr auto hashes = GetSystemsHashArray<ResourceSystem, InputSystem, PhysicsSystem>();
         * static_assert(hashes.size() == 3);
         *
         * // Use in dependency declaration
         * ecs::DependentSystems GetDependents() const override {
         *     static constexpr auto s_dependents = GetSystemsHashArray<ResourceSystem, InputSystem>();
         *     return {s_dependents.cbegin(), s_dependents.size()};
         * }
         * @endcode
         *
         * @see getSystemHash
         * @see ECS_DEPENDENT_SYSTEMS macro
         */
        template <typename... SystemsArgs>
        static constexpr std::array<systemHash_t, sizeof...(SystemsArgs)> GetSystemsHashArray();

    public:
        /**
         * @brief Returns the factory registration names for this component/system
         * @return constexpr std::array<std::string, 0> Empty array (no registration names)
         *
         * This function should be overridden in derived classes to provide one or more
         * factory names under which the component/system can be instantiated.
         *
         * @note By default returns an empty array, meaning no factory registration
         * @note Override this function in derived classes using ECS_REGISTRY macro
         * @note Must be static constexpr to allow compile-time registration
         *
         * @see ECS_REGISTRY macro for convenient override
         *
         * @example
         * @code
         * struct MySystem : ISystem<MySystem> {
         *     static constexpr auto GetRegistryNames() {
         *         return std::array<std::string_view, 2>{"my_system", "alt_name"};
         *     }
         * };
         * @endcode
         */
        static constexpr auto GetRegistryNames()
        {
            return std::array<std::string, 0>();
        }

        /**
         * @brief Static flag that triggers automatic system registration
         *
         * This static member's initialization causes the system type to be
         * automatically registered with the SystemRegistrator.
         */
        [[maybe_unused]] static inline const auto RegisterInfo = SystemRegistrator::Register<SystemCls>();
    };

} // namespace ecs


/**
 * @def ECS_EVENT
 * @brief Macro for declaring and initializing an event listener member
 *
 * @param _METHOD_NAME Name of the event handler method
 * @param _EVENT Type of the event to listen for
 *
 * This macro simplifies the declaration of event listeners in system classes.
 * It declares a private unique_ptr member for the listener and initializes it
 * using RegisterEvent, then restores public access.
 *
 * @par Usage example:
 * @code
 * class MySystem : public ISystem<MySystem>
 * {
 * private:
 *     ECS_EVENT(onPlayerDied, PlayerDiedEvent)
 *     void onPlayerDied(Registry& registry, const PlayerDiedEvent& event);
 *
 *     ECS_EVENT(onScoreChanged, ScoreChangedEvent)
 *     void onScoreChanged(Registry& registry, const ScoreChangedEvent& event);
 * };
 * @endcode
 *
 * @note The macro temporarily changes access to private, then restores public
 * @warning The macro must be used inside the class body
 * @see RegisterEvent
 */
#define ECS_EVENT(_METHOD_NAME, _EVENT) \
    private: \
    std::unique_ptr<Super::EventListener<_EVENT>> m_listener_##_EVENT = Super::RegisterEvent<_EVENT>(&SelfSystemCls::_METHOD_NAME); \
    public:


/**
 * @brief Macro to generate GetDependents() override implementation.
 *
 * This macro creates a GetDependents() method that returns a DependentSystems object
 * containing the hashes of the specified system types. It's designed for use in
 * system classes that need to declare their dependencies on other systems.
 *
 * @param ... The system types that this system depends on.
 *
 * @example
 * @code
 * class RenderSystem : public ISystem<RenderSystem> {
 *     ECS_DEPENDENT_SYSTEMS(ResourceSystem, InputSystem, PhysicsSystem)
 * };
 * @endcode
 */
#define ECS_DEPENDENT_SYSTEMS(...)                                          \
    ecs::DependentSystems GetDependents() const override {                  \
    static const auto s_dependents = GetSystemsHashArray<__VA_ARGS__>();    \
    return {s_dependents.data(), s_dependents.size() }; }


/**
 * @brief Macro to define factory registration names for an ECS component
 *
 * This macro should be used within a component class definition to specify
 * one or more factory names under which the component will be registered.
 * The names enable factory-based creation and lookup of the component type.
 *
 * @param ... One or more string literals representing factory names
 *
 * @note The macro:
 *       1. Defines GetRegistryNames() as a public static constexpr function
 *       2. Returns a std::array<std::string, N> with the specified names
 *
 * @warning Must be used inside a component class derived from IComponent<T>
 * @warning Names must be string literals (compile-time constants)
 *
 * @see IComponent
 * @see SystemRegistrator::Register
 *
 * @example
 * @code
 * class MyComponent : public IComponent<MyComponent> {
 *     ECS_REGISTRY("my_component", "alt_name")
 * };
 * @endcode
 */
#define ECS_REGISTRY(...)                                                                                   \
public:                                                                                                     \
static constexpr auto GetRegistryNames() {                                                                  \
return std::array<std::string, sizeof((const char*[]){__VA_ARGS__}) / sizeof(const char*)>{__VA_ARGS__};}

#endif
#include "systems/detail/ISystem.ipp"
