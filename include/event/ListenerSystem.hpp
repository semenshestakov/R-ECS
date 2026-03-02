#pragma once
#include "AbstractListener.hpp"
#include "Listener.hpp"


namespace event
{


    /**
     * @class ListenerSystem
     * @brief Centralized system for managing multiple smart listeners with key-based access.
     *
     * Provides a dictionary-like interface for managing event subscriptions where each
     * subscription is identified by a key. Automatically handles cleanup of all managed
     * listeners upon destruction.
     *
     * @tparam K The key type, must be key for use with std::unordered_map
     * @tparam T The callback identifier storage type for the managed listeners
     *
     * @note Non-copyable and non-movable to maintain ownership semantics
     * @note Thread safety: Not thread-safe by default. External synchronization required
     *       for concurrent access.
     *
     * @example
     * // Create a listener system using string keys
     * SmartListenerSystemT<std::string, callbackId_t> listenerSystem;
     *
     * // Subscribe to an event with key "player_moved"
     * listenerSystem.subscribe("player_moved", &movementEvent,
     *     [](float x, float y) {
     *         std::cout << "Player moved to: " << x << ", " << y << std::endl;
     *     });
     *
     * // Later, unsubscribe by key
     * listenerSystem.unsubscribe("player_moved");
     */
    template<typename K /* key */, typename T /* abstract_smart_listener_type */>
    class ListenerSystem
    {
    public:
        /**
         * @brief Template alias for creating typed listeners.
         * @tparam Args Event argument types
         */
        using AbstractSmartListener_t = AbstractListener<T>;
        template<typename... Args> using SmartListener_t = Listener<T, Args...>;

        /**
         * @brief Default constructor.
         */
        ListenerSystem();

        /**
         * @brief Constructs a listener system with custom deleter for all managed listeners.
         * @param deleter The deleter to use for all listeners created by this system
         */
        explicit ListenerSystem(const deleter_t &deleter);

        /**
         * @brief Destructor automatically unsubscribes all managed listeners.
         */
        ~ListenerSystem();

        // delete copy | move
        ListenerSystem(const ListenerSystem&) = delete;
        ListenerSystem& operator=(const ListenerSystem&) = delete;
        ListenerSystem(ListenerSystem&&) = delete;
        ListenerSystem& operator=(ListenerSystem&&) = delete;

        /**
         * @brief Subscribes to an event with a unique key identifier.
         *
         * Creates a new smart listener for the specified event and callback, storing it
         * in the internal map under the provided key. If a listener already exists for
         * this key, it will be replaced (after proper cleanup of the old listener).
         *
         * @tparam Args The event argument types
         * @param key Unique identifier for this subscription
         * @param event Pointer to the event to subscribe to
         * @param callback The callback function to register
         *
         * @note The system takes ownership of the created listener
         */
        template<typename... Args> void subscribe(const K& key, Event<Args...>* event, eventCallback_t<Args...> callback);

        /**
         * @brief Unsubscribes and removes a listener by its key.
         *
         * @param key The key identifying the listener to remove
         *
         * @note If no listener exists for the key, this method does nothing (no-op)
         * @note The listener is properly destroyed and all its callbacks are unregistered
         */
        void unsubscribe(const K& key);

    private:
        /// Map of key to listener pointers. Uses unique_ptr for automatic memory management.
        std::unordered_map<K, std::unique_ptr<AbstractSmartListener_t>> m_mapListeners;

        /// Optional custom deleter applied to all listeners created by this system
        deleter_t m_deleter;
    };

}

#include "detail/ListenerSystem.ipp"

