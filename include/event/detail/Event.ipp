#pragma once
#include "../Event.hpp"


namespace event
{

    template<typename... Args>
    callbackId_t Event<Args...>::add(const Callback& callback)
    {
        callbackId_t callbackId = m_lastCallbackId++;
        m_callbacksMap[callbackId] = callback;
        return callbackId;
    }

    template<typename... Args>
    AbstractEvent* Event<Args...>::New() const
    {
       return new Event();
    }

    template<typename... Args>
    void Event<Args...>::remove(const callbackId_t& callbackId)
    {
        if (callbackId == INVALID_EVENT_ID)
            return;
            
        auto it = m_callbacksMap.find(callbackId);
        if (it != m_callbacksMap.end())
        {
            m_callbacksMap.erase(it);
        }
    }

    template<typename... Args>
    void Event<Args...>::operator()(Args... args)
    {
        for (auto& [callbackId, callback] : m_callbacksMap)
        {
            if (callback)
            {
                callback(std::forward<Args>(args)...);
            }
        }
    }

} // end namespace Event 
