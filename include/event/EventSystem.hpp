#pragma once
#include <string_view>
#include <unordered_map>
#include "event/Event.hpp"



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
        EventSystem(EventSystem&&) = delete;
        EventSystem& operator=(EventSystem&&) = delete;

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
        void on(const K& key, Args&&... args);

        template<class... Args>
        Event<Args...>* get(const K& key);

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
#include "detail/EventSystem.ipp"
