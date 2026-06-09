#ifndef EVENT_SYSTEM_HPP
#define EVENT_SYSTEM_HPP
#include <unordered_map>
#include "collections/CommandQueue.hpp"
#include "event/Event.hpp"
#include "event/ListenerSystem.hpp"


namespace event
{
    struct AbstractEvent;


    /**
     * @brief Generic event system that uses key keys to identify events
     *
     * @tparam K The key type used as event key (e.g., std::string, enum class, int)
     */
    template<typename K /* key */>
    class EventSystem
    {
    public:
        EventSystem();
        ~EventSystem();

        // Non-copyable, non-movable
        EventSystem(const EventSystem&) = delete;
        EventSystem& operator=(const EventSystem&) = delete;
        EventSystem(EventSystem&&) = default;
        EventSystem& operator=(EventSystem&&) = default;

        /**
         * @brief Create a new event with the specified key and signature
         *
         * @tparam Args Event argument types
         * @param key Key to identify the event
         * @return true if event was created, false if it already exists
         */
        template<typename... Args>
        [[maybe_unused]] bool Create(const K& key);

        /**
         * @brief Delete an event by its key
         *
         * @param key Key of the event to delete
         */
        void Delete(const K& key);

        /**
         * @brief Remove all events
         */
        void clear();

        /**
         * @brief Trigger an event with the specified arguments
         *
         * @tparam Args Event argument types
         * @param key Key of the event to trigger
         * @param args Arguments to pass to the event callbacks
         *
         * @note If the event doesn't exist or has wrong signature, this function does nothing
         */
        template<class... Args>
        void OnEvent(const K& key, Args&&... args);

        /**
         * @brief Enqueue an event for deferred processing within the current frame
         *
         * Schedules an event invocation to be executed later during the event
         * processing phase (typically at a well-defined point in the frame).
         * This allows systems to emit events without immediately affecting
         * execution flow, ensuring deterministic behavior across systems.
         *
         * @tparam Args Event argument types
         * @param key Identifier of the event to enqueue
         * @param args Arguments forwarded to event listeners
         *
         * @note Events are stored in a FIFO queue and processed in insertion order
         * @note Arguments are captured using perfect forwarding and may be moved
         * @note If the event is not registered or has incompatible signature,
         *       it will be safely ignored during processing
         *
         * @warning Deferred execution means listeners will not observe the event
         *          until FlushEvents() is called
         */
        template<class... Args>
        void PushEvent(const K& key, Args&&... args);

        /**
         * @brief Process all queued events
         *
         * Executes all events accumulated via PushEvent() in a deterministic,
         * FIFO order. This function represents the event dispatch phase of the
         * frame and should typically be called once per frame at a controlled
         * synchronization point.
         *
         * Ensures that:
         * - All systems observe events in the same order
         * - No mid-update side effects occur from immediate event dispatch
         * - Event-driven logic remains deterministic and frame-consistent
         *
         * After execution, the internal event queue is cleared.
         *
         * @note Safe to call multiple times; has no effect if the queue is empty
         * @warning Events generated during FlushEvents() are not processed
         */
        void FlushEvents();

        /**
         * @brief Retrieve an event by key if it exists
         *
         * @tparam Args Event argument types
         * @param key Key of the event to retrieve
         * @return Pointer to the event if found, otherwise nullptr
         *
         * @note Safe lookup — returns nullptr if event is not registered
         */
        template<class... Args>
        Event<Args...>* TryGet(const K& key);

        template<class... Args>
        const Event<Args...>* TryGet(const K& key) const;

        /**
         * @brief Retrieve a reference to an event by key
         *
         * @tparam Args Event argument types
         * @param key Key of the event to retrieve
         * @return Reference to the event
         *
         * @throws assert if event with given key does not exist
         *
         * @note Use when the event is guaranteed to exist
         */
        template<class... Args>
        Event<Args...>& Get(const K& key);

        template<class... Args>
        const Event<Args...>& Get(const K& key) const;

        /**
         * @brief Check if an event with the given key exists
         *
         * @param key Key to check
         * @return true if event exists, false otherwise
         */
        [[nodiscard]] bool contains(const K& key) const;

        /**
         * @brief Get the number of registered events
         */
        [[nodiscard]] size_t size() const;

    protected:
        using absEventPtr_t = std::unique_ptr<AbstractEvent>;

        /**
         * @brief Registry of all event channels
         *
         * Maps event keys to their corresponding event instances.
         * Each entry defines a unique event type and its associated listeners.
         *
         * This container represents the persistent event infrastructure
         * shared across systems.
         */
        std::unordered_map<K, absEventPtr_t> m_eventsMap;

        /**
         * @brief Deferred event execution queue
         *
         * Stores callable units representing scheduled event invocations.
         * Each entry encapsulates:
         * - Event key
         * - Bound arguments
         * - Dispatch logic
         *
         * The queue is processed sequentially during FlushEvents(),
         * guaranteeing deterministic event ordering across the frame.
         */
        collections::CommandQueue m_commandQueue;
    };

} // namespace event
#endif
#include "detail/EventSystem.ipp"
