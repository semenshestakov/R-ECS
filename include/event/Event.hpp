#pragma once
#include <map>
#include <any>

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
        struct Entry
        {
            callbackId_t id{};
            int priority{};
            Callback callback;
            bool removed = false;
        };

        Event() = default;
        ~Event() override = default;

        // Non-copyable, non-movable
        Event(const Event&) = delete;
        Event& operator=(const Event&) = delete;
        Event(Event&&) = delete;
        Event& operator=(Event&&) = delete;

        /**
         * Register a callback for this event.
         * @param callback Function to call when event is triggered
         * @param priority Priority
         * @return Unique callback ID for later removal
         */
        callbackId_t add(const Callback& callback, int priority = 0);

        /**
         * @brief Factory method to create concrete event instance.
         */
        [[nodiscard]] AbstractEvent* New() const override;

        /**
         * Remove callback by ID.
         * @param callbackId ID returned by add()
         */
        void remove(callbackId_t callbackId) override;

        /**
         * Trigger the event, calling all registered callbacks.
         * @param args Arguments to pass to callbacks
         */
        void operator()(Args... args);
    
    private:
        std::vector<Entry> m_callbacks;                                                 ///< Callback storage (sorted by ID)
        callbackId_t m_lastCallbackId = INVALID_CALLBACK_ID + 1;                        ///< Next callback ID
        bool m_dispatching = false;

    };

}

#include "detail/Event.ipp"
