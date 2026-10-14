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
    void EventSystem<K>::OnEvent(const K& key, Args&&... args) const
    {
        if(const auto* event = TryGet<Args...>(key))
            (*event)(std::forward<Args>(args)...);
    }

    template<typename K>
    template<class... Args>
    void EventSystem<K>::PushEvent(const K& key, Args&&... args)
    {
        m_commandQueue.Push(
            [this, key, argsTuple = std::make_tuple(std::forward<Args>(args)...)]() mutable
            {
                std::apply(
                    [this, &key]<typename... T0>(T0&&... unpackedArgs)
                    {
                        this->OnEvent(key, std::forward<T0>(unpackedArgs)...);
                    },
                    std::move(argsTuple)
                );
            });
    }

    template<typename K>
    void EventSystem<K>::FlushEvents()
    {
        m_commandQueue.Flush();
    }

    template<typename K>
    template<class... Args>
    Event<Args...>* EventSystem<K>::TryGet(const K& key)
    {
        const auto it = m_eventsMap.find(key);
        if(it == m_eventsMap.end())
            return nullptr;

        return dynamic_cast<Event<Args...>*>(it->second.get());
    }

    template<typename K>
    template<class... Args>
    const Event<Args...>* EventSystem<K>::TryGet(const K& key) const
    {
        const auto it = m_eventsMap.find(key);
        if(it == m_eventsMap.end())
            return nullptr;

        return dynamic_cast<const Event<Args...>*>(it->second.get());
    }

    template<typename K>
    template<class... Args>
    Event<Args...>& EventSystem<K>::Get(const K& key)
    {
        Event<Args...>* event = TryGet<Args...>(key);
        assert(event != nullptr);
        return *event;
    }

    template<typename K>
    template<class... Args>
    const Event<Args...>& EventSystem<K>::Get(const K& key) const
    {
        const Event<Args...>* event = TryGet<Args...>(key);
        assert(event != nullptr);
        return *event;
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
