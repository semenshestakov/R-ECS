#pragma once
#include <algorithm>
#include <utility>

#include "AbstractListener.hpp"
#include "Event.hpp"


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
        using Event_t = Event<Args...>;                                ///< Type alias for the specific event type

        /**
         * @brief Default constructor creating an empty listener.
         */
        Listener() : Listener(nullptr) {}

        /**
         * @brief Constructs a listener associated with an event but without callbacks.
         * @param event Pointer to the event. Callbacks can be added later with addCallback().
         */
        explicit Listener(Event_t* event) : Listener(event, nullptr) {}

        /**
         * @brief Constructs a listener with a single initial callback.
         * @param event Pointer to the event
         * @param callback The callback function to register immediately
         */
        Listener(Event_t* event, const Callback_t callback) : Listener(event, callback, nullptr) {}

        /**
         * @brief Constructs a listener with callback and custom deleter.
         * @param event Pointer to the event
         * @param callback The callback function to register
         * @param deleter Custom deleter for extended cleanup
         */
        Listener(Event_t* event, Callback_t callback, const deleter_t &deleter);

        /**
         * @brief Constructs a listener with multiple initial callbacks.
         * @param event Pointer to the event
         * @param callbacks Vector of callbacks to register immediately
         */
        Listener(Event_t* event, const std::vector<Callback_t>&& callbacks): Listener(event, std::move(callbacks), nullptr) {}

        /**
         * @brief Constructs a listener with multiple callbacks and custom deleter.
         * @param event Pointer to the event
         * @param callbacks Vector of callbacks to register
         * @param deleter Custom deleter for extended cleanup
         */
        Listener(Event_t* event, const std::vector<Callback_t>&& callbacks, deleter_t deleter);

        // Non-copyable
        Listener(const Listener&) = delete;
        Listener& operator=(const Listener&) = delete;

        /**
         * @brief Move constructor.
         * Efficiently transfers callback ownership from another listener.
         * @param other The listener to move from. Will be left in valid but empty state.
         */
        Listener(Listener && other) noexcept { this->swap(std::move(other)); }

        /**
         * @brief Move assignment operator.
         * @param other The listener to move from
         * @return Reference to this listener
         */
        Listener &operator=(Listener && other) noexcept { this->swap(std::move(other)); return *this;}

        /**
         * @brief Swaps contents with another listener.
         * Efficiently exchanges all internal state including event association,
         * callback IDs, and deleter.
         * @param other The listener to swap with
         */
        void swap(Listener && other);

        /**
         * @brief Adds an additional callback to this listener.
         * @param callback The callback function to add
         */
        void addCallback(Callback_t callback);
    };

}


#include "detail/Listener.ipp"
