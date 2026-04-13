#ifndef LISTENER_HPP
#define LISTENER_HPP
#include "AbstractListener.hpp"


namespace event
{

    /**
     * @class Listener
     * @brief Concrete typed smart listener for specific event signatures.
     *
     * Provides type-safe event subscription with automatic callback management. Callbacks
     * are registered on construction and automatically unregistered on destruction.
     * Supports move semantics for efficient transfer of callback ownership.
     *
     * @tparam T Callback identifier storage type (inherited from AbstractListener)
     * @tparam Args Variadic template parameter representing the event argument types
     *
     * @note Final class - not intended for further derivation
     * @note Non-copyable to prevent duplicate callback registration
     * @note Supports move construction and assignment for efficient resource transfer
     *
     * @example
     * // Create a listener for EventT<int, std::string> with single callback
     * SmartListenerT<callbackId_t, int, std::string> listener(&myEvent,
     *     [](int value, const std::string& text) {
     *         std::cout << value << ": " << text << std::endl;
     *     });
     *
     * // Create a listener with multiple initial callbacks
     * std::vector<eventCallback_t<int>> callbacks = {callback1, callback2};
     * SmartListenerT<std::vector<callbackId_t>, int> multiListener(&intEvent, std::move(callbacks));
     */
    template<typename T, typename... Args>
    class Listener final : public AbstractListener<T>
    {
    public:
        using Callback_t = eventCallback_t<Args...> ;                   ///< Type alias for the callback signature
        using Event_t = Event<Args...>;                                 ///< Type alias for the specific event type

        /**
         * @brief Default constructor creating an empty listener.
         */
        Listener();

        /**
         * @brief Constructs a listener associated with an event but without callbacks.
         * @param event Pointer to the event. Callbacks can be added later with addCallback().
         */
        explicit Listener(Event_t* event);


        /**
         * @brief Constructs a listener with callback and custom deleter.
         * @param event Pointer to the event
         * @param callback The callback function to register
         * @param deleter Custom deleter for extended cleanup
         */
        Listener(Event_t* event, const Callback_t& callback, const deleter_t& deleter = nullptr);

        /**
         * @brief Constructs a listener with multiple callbacks and custom deleter.
         * @param event Pointer to the event
         * @param callbacks Vector of callbacks to register
         * @param deleter Custom deleter for extended cleanup
         */
        Listener(Event_t* event, const std::vector<Callback_t>& callbacks, const deleter_t& deleter = nullptr);

        /**
         * @brief Constructs a listener with multiple callbacks and custom deleter.
         * @param event Pointer to the event
         * @param callbacksWithPriority Vector of Callbacks & Priority to register
         * @param deleter Custom deleter for extended cleanup
         */
        Listener(Event_t* event, const std::vector<std::pair<Callback_t, int>>& callbacksWithPriority, const deleter_t& deleter = nullptr);

        // Non-copyable
        Listener(const Listener&) = delete;
        Listener& operator=(const Listener&) = delete;

        /**
         * @brief Move constructor.
         * Efficiently transfers callback ownership from another listener.
         * @param other The listener to move from. Will be left in valid but empty state.
         */
        Listener(Listener&& other) noexcept;

        /**
         * @brief Move assignment operator.
         * @param other The listener to move from
         * @return Reference to this listener
         */
        Listener& operator=(Listener&& other) noexcept;

        /**
         * @brief Swaps contents with another listener.
         * Efficiently exchanges all internal state including event association,
         * callback IDs, and deleter.
         * @param other The listener to swap with
         */
        void swap(Listener&& other);

        /**
         * @brief Adds a callback to this listener.
         * @param callback The callback function to add
         * @param priority The callback priority
         */
        void subscribe(const Callback_t& callback, int priority = 0);

        /**
         * @brief Adds a callback using a specified event.
         * @param event Pointer to the event to subscribe to
         * @param callback The callback function to add
         * @param priority The callback priority
         *
         * @note This updates the listener's associated event to the provided one.
         *       Any previously associated event remains unchanged but the listener
         *       will now use this new event for future operations.
         */
        void subscribe(Event_t* event, const Callback_t& callback, int priority = 0);
    };

    /**
     * @brief Alias template for a Listener that manages a single callback.
     *
     * This alias provides a convenient shorthand for creating listeners that handle
     * exactly one callback per event type. It uses `callbackId_t` as the storage type,
     * which is optimized for single callback management.
     *
     * @tparam Args The event argument types that the callback will receive
     *
     * @details
     * `SingleListener` is ideal for scenarios where you need to listen to an event
     * with just one callback. It's more lightweight than collection-based listeners
     * and provides simpler semantics when you know you'll only need one callback
     * per listener instance.
     *
     * Key characteristics:
     * - Manages exactly one callback ID internally
     * - Automatically unregisters the callback on destruction (RAII)
     * - Non-copyable, but movable
     * - Supports custom deleters for extended cleanup
     *
     * @see Listener
     * @see callbackId_t
     *
     * @par Example usage:
     * @code
     * // Create an event that takes an int parameter
     * Event<int> intEvent;
     *
     * // Create a single callback listener
     * SingleListener<int> listener(&intEvent, [](int value) {
     *     std::cout << "Received: " << value << std::endl;
     * });
     *
     * // The callback will be automatically unregistered when listener
     * // goes out of scope
     * @endcode
     *
     * @par Comparison with MultiListener:
     * @code
     * // Single callback - use SingleListener
     * SingleListener<int, std::string> single(&event, callback);
     *
     * @endcode
     */
    template<typename... Args>
    using SingleListener = Listener<callbackId_t, Args...>;
}
#endif

#include "detail/Listener.ipp"
