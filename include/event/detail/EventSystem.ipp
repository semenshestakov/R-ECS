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
    bool EventSystem<K>::Create(const K& key)
    {
        if(m_eventsMap.contains(key))
            return false;

        m_eventsMap[key] = std::make_unique<Event<Args...>>();
        return true;
    }

    template<typename K>
    void EventSystem<K>::Delete(const K& key)
    {
        const auto it = m_eventsMap.find(key);
        if(it != m_eventsMap.end())
        {
            m_eventsMap.erase(it);
        }
    }

    template<typename K>
    void EventSystem<K>::clear()
    {
        m_eventsMap.clear();
    }

    template<typename K>
    template<typename... Args>
    void EventSystem<K>::on(const K& key, Args&&... args)
    {
        if(auto* event = get<Args...>(key))
            (*event)(std::forward<Args>(args)...);
    }

    template<typename K>
    template<class... Args>
    Event<Args...>* EventSystem<K>::get(const K& key)
    {
        const auto it = m_eventsMap.find(key);
        if(it == m_eventsMap.end())
            return nullptr;

        return dynamic_cast<Event<Args...>*>(it->second.get());
    }

    template<typename K>
    bool EventSystem<K>::contains(const K& key) const
    {
        return m_eventsMap.contains(key);
    }

    template<typename K>
    size_t EventSystem<K>::size() const
    {
        return m_eventsMap.size();
    }

} // namespace event
