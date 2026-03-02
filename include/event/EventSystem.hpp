#pragma once
#include <string_view>
#include <unordered_map>
#include "event/Event.hpp"



namespace event
{
    struct AbstractEvent;


    /**
     * @brief Generic event system that uses hashable keys to identify events
     *
     * @tparam H The hashable type used as event key (e.g., std::string, enum class, int)
     */
    template<typename H /* hashable_value */>
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
         * @param hashable Key to identify the event
         * @return true if event was created, false if it already exists
         */
        template<typename... Args>
        [[maybe_unused]] bool Create(const H& hashable);

        /**
         * @brief Delete an event by its key
         *
         * @param hashable Key of the event to delete
         */
        void Delete(const H& hashable);

        /**
         * @brief Remove all events
         */
        void clear();

        /**
         * @brief Trigger an event with the specified arguments
         *
         * @tparam Args Event argument types
         * @param hashable Key of the event to trigger
         * @param args Arguments to pass to the event callbacks
         *
         * @note If the event doesn't exist or has wrong signature, this function does nothing
         */
        template<class... Args>
        void on(const H& hashable, Args&&... args);

        template<class... Args>
        Event<Args...>* get(const H& hashable);

        /**
         * @brief Check if an event with the given key exists
         *
         * @param hashable Key to check
         * @return true if event exists, false otherwise
         */
        [[nodiscard]] bool contains(const H& hashable) const;

        /**
         * @brief Get the number of registered events
         */
        [[nodiscard]] size_t size() const;

    private:
        using absEventPtr_t = std::unique_ptr<AbstractEvent>;
        using eventMap_t = std::unordered_map<H, absEventPtr_t>;

        eventMap_t m_eventsMap;
    };

} // namespace event
#include "detail/EventSystem.ipp"
