#pragma once
#include "Utils.hpp"
#include "ecs/jobs/ThreadAffinity.hpp"
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
        /// so that they are not hidden by the templated ECS-specific OnEvent(Event)/PushEvent(Event)/FlushEvents(Token) method
        using Super::OnEvent;
        using Super::PushEvent;
        using Super::FlushEvents;

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
        void OnEvent(const Event& event) const;

        /**
         * @brief Enqueue an ECS event for deferred processing within the frame
         *
         * Schedules an event to be dispatched later during the event processing phase.
         * Unlike OnEvent(), this method does not trigger callbacks immediately,
         * allowing systems to emit events without introducing mid-update side effects.
         *
         * The event is internally converted into a type-safe key using
         * GetEventKey<Event>() and queued together with the Registry reference.
         *
         * @tparam Event Type of the ECS event
         * @param event Event instance containing payload data
         *
         * @note Events are processed in FIFO order during FlushEvents()
         * @note Event arguments may be moved to avoid unnecessary copies
         * @warning The event will not be visible to subscribers until the flush phase
         */
        template<class Event>
        void PushEvent(Event&& event);

        /**
         * @brief Restricted token used to control event queue flushing
         *
         * This token enforces that only authorized systems (e.g., Registry)
         * can trigger event queue processing. It prevents accidental or
         * out-of-order flushing from arbitrary code, preserving deterministic
         * execution guarantees.
         *
         * @note Only Registry is allowed to construct this token
         */
        struct FlushEventsToken
        {
            friend class Registry;
            FlushEventsToken() = default;
        };

        /**
         * @brief Execute all deferred ECS events
         *
         * Processes all events previously enqueued via PushEvent() in a
         * deterministic FIFO order. Each event is dispatched to all
         * subscribed systems with the Registry automatically injected.
         *
         * This method represents the event dispatch phase of the ECS frame
         * and is typically called once per frame by the Registry.
         *
         * @param _ Authorization token restricting access to this method
         *
         * @note Safe to call when the queue is empty
         * @warning Events pushed during this call are not processed in the same pass
         */
        void FlushEvents(FlushEventsToken _);

        /**
         * @brief Subscribe to an ECS event by its type, receiving just the event payload.
         *
         * Mirror of OnEvent()/PushEvent(): the event type is turned into a compile-time
         * key via GetEventKey<Event>(), and the underlying (Registry&, const Event&)
         * channel is created on demand. The handler is invoked with only the event, which
         * is the convenient single-argument form.
         *
         * @tparam Event ECS event type to listen for.
         * @param callback Handler invoked as void(const Event&).
         * @param priority Listener priority (higher runs first).
         * @return Callback id, usable with Unsubscribe().
         */
        template<class Event>
        event::callbackId_t Subscribe(std::function<void(const Event&)> callback, event::priority_t priority = event::DEFAULT_PRIORITY);

        /**
         * @brief Subscribe to an ECS event, also receiving the owning Registry.
         *
         * @tparam Event ECS event type to listen for.
         * @param callback Handler invoked as void(Registry&, const Event&).
         * @param priority Listener priority (higher runs first).
         * @return Callback id, usable with Unsubscribe().
         */
        template<class Event>
        event::callbackId_t Subscribe(std::function<void(Registry&, const Event&)> callback,
                                      event::priority_t priority = event::DEFAULT_PRIORITY);

        /**
         * @brief Remove a previously registered listener for @p Event.
         */
        template<class Event>
        void Unsubscribe(event::callbackId_t callbackId);

    private:
        std::reference_wrapper<Registry> m_registryRef;     ///< Reference to the ECS registry used for event callbacks
    };


    template<class Event>
    void EventSystem::OnEvent(const Event& event) const
    {
        ECS_ASSERT_MAIN_THREAD("EventSystem::OnEvent");
        Super::OnEvent<Registry&, const Event&>(GetEventKey<Event>(), m_registryRef.get(), event);
    }

    template<class Event>
    void EventSystem::PushEvent(Event&& event)
    {
        ECS_ASSERT_MAIN_THREAD("EventSystem::PushEvent");
        using Decayed = std::decay_t<Event>;
        m_commandQueue.Push(
            [this, ev = std::forward<Event>(event)]() mutable
            {
                EventSystem::OnEvent<Decayed>(ev);
            });
    }

    inline void EventSystem::FlushEvents(FlushEventsToken _)
    {
        ECS_ASSERT_MAIN_THREAD("EventSystem::FlushEvents");
        Super::FlushEvents();
    }

    template<class Event>
    event::callbackId_t EventSystem::Subscribe(std::function<void(Registry&, const Event&)> callback, const event::priority_t priority)
    {
        ECS_ASSERT_MAIN_THREAD("EventSystem::Subscribe");
        const eventKey_t key = GetEventKey<Event>();
        Super::Create<Registry&, const Event&>(key); // no-op if the channel already exists
        return Super::Get<Registry&, const Event&>(key).add(std::move(callback), priority);
    }

    template<class Event>
    event::callbackId_t EventSystem::Subscribe(std::function<void(const Event&)> callback, const event::priority_t priority)
    {
        return Subscribe<Event>(
            std::function<void(Registry&, const Event&)>(
                [cb = std::move(callback)](Registry&, const Event& event) { cb(event); }),
            priority);
    }

    template<class Event>
    void EventSystem::Unsubscribe(const event::callbackId_t callbackId)
    {
        ECS_ASSERT_MAIN_THREAD("EventSystem::Unsubscribe");
        if (auto* channel = Super::TryGet<Registry&, const Event&>(GetEventKey<Event>()))
            channel->remove(callbackId);
    }

} // namespace ecs
