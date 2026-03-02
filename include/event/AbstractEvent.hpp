#pragma once
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
        ~AbstractEvent() = default;

        ///< Unique event identifier (read-only)
        const eventId_t id;

        /**
         * Remove a callback by its ID.
         * @param callbackId ID of callback to remove
         */
        virtual void remove(const callbackId_t& callbackId) = 0;
        virtual callbackId_t addAny(const std::any& callback) = 0;

    private:
        /// Static counter for unique IDs
        inline static eventId_t s_lastEventId = 1;
    };

}