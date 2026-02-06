#pragma once
#include "../ListenerSystem.hpp"


namespace event
{

    template<typename H, typename T>
    ListenerSystem<H, T>::ListenerSystem() : ListenerSystem(nullptr) {}

    template<typename H, typename T>
    ListenerSystem<H, T>::ListenerSystem(const deleter_t& deleter) : m_deleter(deleter) {}

    template<typename H, typename T>
    ListenerSystem<H, T>::~ListenerSystem()
    {
        m_mapListeners.clear();
    }

    template<typename H, typename T> template<typename... Args>
    void ListenerSystem<H, T>::subscribe(const H& key, Event<Args...>* event, eventCallback_t<Args...> callback)
    {
        if (event == nullptr)
            return;

        if (m_mapListeners.contains(key))
        {
            auto* absSmartEvent =  m_mapListeners[key].get();
            if (absSmartEvent->eventId() != event->id)
            {
                throw std::runtime_error("eventId != newEventId");
            }

            auto* smartEvent = dynamic_cast<SmartListener_t<Args...>*>(absSmartEvent);
            if (!smartEvent)
                throw std::runtime_error("smartEvent");

            smartEvent->addCallback(callback);
            return;
        }

        m_mapListeners.emplace(
            key,
            std::unique_ptr<AbstractSmartListener_t>(new SmartListener_t<Args...>(event, callback, m_deleter))
        );
    }

    template<typename H, typename T>
    void ListenerSystem<H, T>::unsubscribe(const H& key)
    {
        if (m_mapListeners.contains(key))
        {
            m_mapListeners.erase(key);
        }
    }

}

