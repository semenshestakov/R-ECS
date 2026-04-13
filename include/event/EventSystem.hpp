#ifndef EVENT_SYSTEM_HPP
#define EVENT_SYSTEM_HPP
#include <unordered_map>
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
    class EventSystem final
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

    private:
        using absEventPtr_t = std::unique_ptr<AbstractEvent>;
        using eventMap_t = std::unordered_map<K, absEventPtr_t>;

        eventMap_t m_eventsMap;
    };

} // namespace event
#endif
#include "detail/EventSystem.ipp"
