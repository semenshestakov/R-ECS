#pragma once
#include <map>

#include "AbstractEvent.hpp"
#include "EventUtils.hpp"


namespace event
{

    /**
     * Typed event implementation supporting variadic arguments.
     *
     * @tparam Args... Event argument types
     *
     * @note Final class - not intended for further derivation
     * @note Non-copyable, non-movable
     */
    template<typename... Args>
    struct Event final : AbstractEvent
    {
        /// Callback signature type
        using Callback = eventCallback_t<Args...>;

        Event() = default;
        ~Event() = default;

        // Non-copyable, non-movable
        Event(const Event&) = delete;
        Event& operator=(const Event&) = delete;
        Event(Event&&) = delete;
        Event& operator=(Event&&) = delete;

        /**
         * Register a callback for this event.
         * @param callback Function to call when event is triggered
         * @return Unique callback ID for later removal
         */
        callbackId_t add(const Callback& callback);
        callbackId_t addAny(const std::any& callback) override;

        /**
         * Remove callback by ID.
         * @param callbackId ID returned by add()
         */
        void remove(const callbackId_t& callbackId) override;

        /**
         * Trigger the event, calling all registered callbacks.
         * @param args Arguments to pass to callbacks
         */
        void operator()(Args... args);
    
    protected:
        std::map<callbackId_t, Callback> m_callbacksMap;        ///< Callback storage (sorted by ID)
        callbackId_t m_lastCallbackId = 1;                      ///< Next callback ID
        
    };

}

#include "detail/Event.ipp"
