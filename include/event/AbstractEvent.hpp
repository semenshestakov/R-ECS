#pragma once
#include <any>
#include "event/EventUtils.hpp"


namespace event
{

    /**
     * Base abstract class for all event types in the event system.
     * Provides unique event identification and basic callback management interface.
     */
    struct AbstractEvent
    {
        AbstractEvent() : id(s_lastEventId++) {}                    ///< Auto-assign unique ID
        virtual ~AbstractEvent() = default;

        ///< Unique event identifier (read-only)
        const eventId_t id;

        /**
         * Remove a callback by its ID.
         * @param callbackId ID of callback to remove
         */
        virtual void remove(const callbackId_t& callbackId) = 0;

        /**
         * @brief Factory method to create concrete event instance.
         *
         * Derived classes return their specific Event<Ts...> implementation.
         * Ensures proper polymorphic ownership via AbstractEvent* interface.
         *
         * @return New concrete event instance (ownership transferred to caller)
         * @nodiscard Ensures factory result is not ignored
         */
        [[nodiscard]] virtual AbstractEvent* New() const = 0;

    private:
        /// Static counter for unique IDs
        inline static eventId_t s_lastEventId = 1;
    };

}