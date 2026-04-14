#pragma once
#include "ecs/utils/SystemUtils.hpp"
#include "event/EventSystem.hpp"


namespace ecs
{
    class Registry;


    /**
     * @brief ECS-specific event system built on top of the generic event dispatcher.
     *
     * This class acts as a bridge between the ECS Registry and the underlying
     * event system implementation. It binds ECS event types to strongly-typed
     * dispatch calls using compile-time event keys.
     *
     * It extends the base event system by automatically injecting the Registry
     * reference into all event callbacks.
     */
    class EventSystem final : public event::EventSystem<eventKey_t>
    {
        using Super = event::EventSystem<eventKey_t>;

        /// Bring base class overloads of OnEvent into the current scope
        /// so that they are not hidden by the templated ECS-specific OnEvent(Event) method.
        using Super::OnEvent;

    public:
        /**
         * @brief Deleted default constructor.
         *
         * Ensures that the EventSystem is always bound to a valid ECS Registry
         * and cannot exist in an uninitialized state.
         */
        EventSystem() = delete;

        /**
         * @brief Constructs an EventSystem bound to a specific ECS Registry.
         *
         * The registry reference is stored and automatically passed to all
         * event callbacks triggered through this system.
         *
         * @param registryRef Reference to the ECS Registry instance.
         */
        explicit EventSystem(Registry& registryRef) : m_registryRef(registryRef) {}

        /**
         * @brief Dispatches an ECS event to all subscribed systems.
         *
         * Wraps the underlying generic event system call and automatically
         * injects the ECS Registry reference as the first callback argument.
         *
         * The event type is converted into a unique compile-time key using
         * GetEventKey<Event>() to ensure type-safe dispatching.
         *
         * @tparam Event Type of the ECS event being dispatched.
         * @param event Event instance containing payload data.
         */
        template<class Event>
        void OnEvent(const Event& event);

    private:
        std::reference_wrapper<Registry> m_registryRef;     ///< Reference to the ECS registry used for event callbacks
    };


    template<class Event>
    void EventSystem::OnEvent(const Event& event)
    {
        Super::OnEvent<Registry&, const Event&>(GetEventKey<Event>(), m_registryRef.get(), event);
    }

} // namespace ecs
