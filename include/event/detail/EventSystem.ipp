#pragma once
#include "../EventSystem.hpp"


namespace event
{

    template<typename H>
    EventSystem<H>::EventSystem() = default;

    template<typename H>
    EventSystem<H>::~EventSystem() = default;

    template<typename H>
    template<typename... Args>
    bool EventSystem<H>::Create(const H& hashable)
    {
        if(m_eventsMap.contains(hashable))
        {
            auto* event = dynamic_cast<Event<Args...>*>(m_eventsMap[hashable].get());
            return event != nullptr;
        }
        m_eventsMap[hashable] = std::make_unique<Event<Args...>>();
        return true;
    }

    template<typename H>
    void EventSystem<H>::Delete(const H& hashable)
    {
        if(m_eventsMap.contains(hashable))
        {
            m_eventsMap.erase(hashable);
        }
    }

    template<typename H>
    void EventSystem<H>::clear()
    {
        m_eventsMap.clear();
    }

    template<typename H>
    template<typename... Args>
    void EventSystem<H>::on(const H& hashable, Args&&... args)
    {
        if(auto* event = get<Args...>(hashable))
            (*event)(std::forward<Args>(args)...);
    }

    template<typename H>
    template<class... Args>
    Event<Args...>* EventSystem<H>::get(const H& hashable)
    {
        if(!m_eventsMap.contains(hashable))
            return nullptr;

        return dynamic_cast<Event<Args...>*>(m_eventsMap[hashable].get());
    }

    template<typename H>
    bool EventSystem<H>::contains(const H& hashable) const
    {
        return m_eventsMap.contains(hashable);
    }

    template<typename H>
    size_t EventSystem<H>::size() const
    {
        return m_eventsMap.size();
    }

} // namespace event
