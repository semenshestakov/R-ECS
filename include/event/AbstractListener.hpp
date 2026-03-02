#pragma once
#include "EventUtils.hpp"
#include "AbstractEvent.hpp"


namespace event
{


    /**
     * @class AbstractListener
     * @brief Abstract base class for smart event listeners with automatic callback lifecycle management.
     *
     * This class provides the foundation for RAII-style event subscription where callbacks are
     * automatically registered and unregistered upon construction and destruction. It supports
     * custom deleters for extended cleanup behavior and can store either single callback IDs
     * or collections of IDs depending on the template parameter.
     *
     * @tparam T The callback identifier storage type. Must satisfy the `ValidCallbackIdType`
     *           concept. Can be either:
     *           - `callbackId_t` for single callback storage
     *           - Any collection type (e.g., `std::vector<callbackId_t>`) for multiple callbacks
     *
     * @note This class is abstract and cannot be instantiated directly.
     * @note Non-copyable to prevent duplicate callback management issues.
     * @see SmartListenerT for the concrete typed implementation
     * @see SmartListenerSystemT for managing multiple listeners
     *
     * @example
     * // Using with single callback ID
     * AbstractListener<callbackId_t> singleListener(event);
     *
     * // Using with multiple callback IDs
     * AbstractListener<std::vector<callbackId_t>> multiListener(event);
     */
    template<ValidCallbackIdType T>
    class AbstractListener
    {
    public:
        /**
         * @brief Default constructor creating a listener not associated with any event.
         */
        AbstractListener();

        /**
         * @brief Constructs a listener associated with a specific event.
         * @param event Pointer to the event to listen to. Can be nullptr for delayed association.
         */
        explicit AbstractListener(AbstractEvent* event);

        /**
         * @brief Constructs a listener with custom deleter for extended cleanup.
         * @param event Pointer to the associated event
         * @param deleter Custom deleter function called during destruction for additional cleanup
         */
        AbstractListener(AbstractEvent* event, const deleter_t& deleter);

        /**
         * @brief Virtual destructor ensures proper cleanup of derived classes.
         * Automatically unregisters all callbacks managed by this listener.
         */
        virtual ~AbstractListener();

        // Non-copyable to prevent callback management conflicts
        AbstractListener(const AbstractListener&) = delete;
        AbstractListener& operator=(const AbstractListener&) = delete;

        /**
         * @brief Merges callbacks from another listener into this one.
         * Transfers callback ownership from `other` to this listener. After this operation,
         * `other` will be empty and should not be used further.
         *
         * @param other Another listener to merge from. Will be left in valid but empty state.
         * @note This is a move operation - `other` loses ownership of its callbacks.
         * @warning `other` must be of the same concrete type as `this`.
         */
        void unionCallbacks(AbstractListener&& other);

        /**
         * @brief Retrieves the unique identifier of the associated event.
         *
         * @return eventId_t The event ID, or 0 if no event is associated.
         *
         * @note Returns 0 if `m_event` is nullptr.
         */
        [[nodiscard]] eventId_t eventId() const;

        /**
         * @brief Removes a callback from the associated event.
         * @param callbackId The callback identifier to remove.
         */
        void removeCallbackId(callbackId_t callbackId);

        /**
         * @brief Adds a callback to the associated event.
         * @param callbackId The callback identifier to add to internal storage.
         */
        void addCallbackId(callbackId_t callbackId);

        void addCallbackAny(const std::any& callback) const;

        void unsubscribeAll();

    protected:
        T m_callbackId {};                                  ///< Storage for callback identifier(s). Can be single ID or collection.
        AbstractEvent* m_event = nullptr;                   ///< Pointer to the associated event. Owned externally, not by this class.

    private:
        deleter_t m_deleter = nullptr;                      ///< Optional custom deleter for extended cleanup operations.
    };

}
#include "detail/AbstractListener.ipp"
