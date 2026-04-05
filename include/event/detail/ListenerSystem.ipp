#pragma once
#include <ranges>
#include "../ListenerSystem.hpp"


namespace event
{

    template<typename K, typename T>
    ListenerSystem<K, T>::ListenerSystem() : ListenerSystem(nullptr) {}

    template<typename K, typename T>
    ListenerSystem<K, T>::ListenerSystem(const deleter_t& deleter) : m_deleter(deleter) {}

    template<typename K, typename T>
    ListenerSystem<K, T>::~ListenerSystem()
    {
        m_mapListeners.clear();
    }

    template<typename K, typename T>
    template<typename... Args>
    bool ListenerSystem<K, T>::reg(const K& key, Event<Args...>* event)
    {
        const auto [isReg, it] = isRegistered(key, event);
        if (isReg)
            return true;

        m_mapListeners.emplace(
            key,
            std::unique_ptr<AbstractSmartListener_t>(new SmartListener_t<Args...>(event, nullptr, m_deleter))
        );
        return true;
    }

    template<typename K, typename T>
    std::pair<bool, typename ListenerSystem<K, T>::mapListeners_t::const_iterator> ListenerSystem<K, T>::isRegistered(const K& key, const AbstractEvent* event) const
    {
        if (event == nullptr)
            return {false, m_mapListeners.end()};

        auto it = m_mapListeners.find(key);
        if (it == m_mapListeners.end())
            return {false, it};

        if (it->second->eventId() != event->id)
            throw std::runtime_error("eventId != newEventId");

        return {true, it};
    }

    template<typename K, typename T> template<typename... Args>
    bool ListenerSystem<K, T>::subscribe(const K& key, Event<Args...>* event, eventCallback_t<Args...> callback)
    {
        if (callback == nullptr)
            return false;

        auto it = m_mapListeners.find(key);
        if (it == m_mapListeners.end())
        {
            if (!reg<Args...>(key, event))
                throw std::runtime_error("!reg<Args...>(key, event)");

            it = m_mapListeners.find(key);
        }

        AbstractSmartListener_t* absListener = it->second.get();
        if (absListener->eventId() != event->id)
        {
            throw std::runtime_error("eventId != newEventId");
        }

        auto* listener = dynamic_cast<SmartListener_t<Args...>*>(absListener);
        if (!listener)
            throw std::runtime_error("!smartEvent");

        listener->subscribe(callback);
        return true;
    }

    template<typename K, typename T>
    bool ListenerSystem<K, T>::subscribeAny(const K& key, AbstractEvent* event, const std::any& callback)
    {
        const auto [isReg, it] = isRegistered(key, event);
        if (!isReg)
            return false;

        AbstractSmartListener_t* absListener = it->second.get();
        if (absListener->eventId() != event->id)
            throw std::runtime_error("eventId != newEventId");

        absListener->addCallbackAny(callback);
        return true;
    }

    template<typename K, typename T>
    void ListenerSystem<K, T>::unsubscribe(const K& key)
    {
        auto it = m_mapListeners.find(key);
        if (it != m_mapListeners.end())
        {
            m_mapListeners.erase(it);
        }
    }

    template<typename K, typename T>
    void ListenerSystem<K, T>::unsubscribeAll()
    {
        for (const auto& eventPtr : m_mapListeners | std::views::values)
        {
            eventPtr->unsubscribeAll();
        }
    }

    template<typename K, typename T>
    void ListenerSystem<K, T>::clear()
    {
        m_mapListeners.clear();
    }

    template<typename K, typename T>
    std::size_t ListenerSystem<K, T>::size() const
    {
        return m_mapListeners.size();
    }

}

