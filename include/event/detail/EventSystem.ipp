#pragma once
#include "../EventSystem.hpp"


namespace event
{

    template<typename K>
    EventSystem<K>::EventSystem() = default;

    template<typename K>
    EventSystem<K>::~EventSystem() = default;

    template<typename K>
    template<typename... Args>
    bool EventSystem<K>::Create(const K& hashable)
    {
        if(m_eventsMap.contains(hashable))
        {
            auto* event = dynamic_cast<Event<Args...>*>(m_eventsMap[hashable].get());
            return event != nullptr;
        }
        m_eventsMap[hashable] = std::make_unique<Event<Args...>>();
        return true;
    }

    template<typename K>
    void EventSystem<K>::Delete(const K& hashable)
    {
        if(m_eventsMap.contains(hashable))
        {
            m_eventsMap.erase(hashable);
        }
    }

    template<typename K>
    void EventSystem<K>::clear()
    {
        m_eventsMap.clear();
    }

    template<typename K>
    template<typename... Args>
    void EventSystem<K>::on(const K& hashable, Args&&... args)
    {
        if(auto* event = get<Args...>(hashable))
            (*event)(std::forward<Args>(args)...);
    }

    template<typename K>
    template<class... Args>
    Event<Args...>* EventSystem<K>::get(const K& hashable)
    {
        if(!m_eventsMap.contains(hashable))
            return nullptr;

        return dynamic_cast<Event<Args...>*>(m_eventsMap[hashable].get());
    }

    template<typename K>
    bool EventSystem<K>::contains(const K& hashable) const
    {
        return m_eventsMap.contains(hashable);
    }

    template<typename K>
    size_t EventSystem<K>::size() const
    {
        return m_eventsMap.size();
    }

} // namespace event
