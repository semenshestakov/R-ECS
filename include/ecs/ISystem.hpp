#pragma once

/**
 * @file ISystem.hpp
 * @brief Base interface template for all ECS systems with automatic event registration
 *
 * This header defines the core system interface that all concrete systems must inherit from.
 * It provides automatic registration capabilities and event handling infrastructure.
 */

#include "common_recs/utils/ClassUtils.hpp"
#include "registry/RegistryRegistrator.hpp"
#include "systems/IBaseSystem.hpp"
#include "event/Listener.hpp"
#include "event/EventSystem.hpp"


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
    protected:
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

    protected:
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
         * @brief Static array of registry names for system registration
         *
         * Can be overridden in derived classes to specify custom registry names.
         * Defaults to empty array.
         */
        static constexpr std::array<std::string_view, 0> ECS_REGISTRY_NAMES = {};

        /**
         * @brief Static flag that triggers automatic system registration
         *
         * This static member's initialization causes the system type to be
         * automatically registered with the RegistryRegistrator.
         */
        [[maybe_unused]] static inline const bool IsRegistered = RegistryRegistrator::RegisterSystem<SystemCls>(SystemCls::ECS_REGISTRY_NAMES);

        /**
         * @brief Collection of event registration functions
         *
         * Stores lambdas that will register event listeners when the event system
         * becomes available during system initialization.
         */
        std::vector<std::function<void(EventSystem&)>> m_registerEventFunctions;
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

#include "systems/detail/ISystem.ipp"
