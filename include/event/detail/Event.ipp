#pragma once
#include <algorithm>
#include "../Event.hpp"


namespace event
{

    template<typename... Args>
    callbackId_t Event<Args...>::add(const Callback& callback, const priority_t priority /* = DEFAULT_PRIORITY */)
    {
        callbackId_t id = m_lastCallbackId++;

        auto it = std::upper_bound(
            m_callbacks.begin(),
            m_callbacks.end(),
            priority,
            [](int p, const Entry& e)
            {
                return p > e.priority;
            });

        m_callbacks.insert(it, { id, priority, callback });

        return id;
    }

    template<typename... Args>
    AbstractEvent* Event<Args...>::New() const
    {
       return new Event();
    }

    template<typename... Args>
    void Event<Args...>::remove(const callbackId_t callbackId)
    {
        if (callbackId == INVALID_CALLBACK_ID)
            return;

        if (m_dispatching)
        {
            for (auto& e : m_callbacks)
            {
                if (e.id == callbackId)
                {
                    e.removed = true;
                    return;
                }
            }
        }
        else
        {
            auto it = std::remove_if(
                m_callbacks.begin(),
                m_callbacks.end(),
                [callbackId](const Entry& e)
                {
                    return e.id == callbackId;
                });

            if (it != m_callbacks.end())
                m_callbacks.erase(it, m_callbacks.end());
        }
    }

    template<typename... Args>
    void Event<Args...>::operator()(Args... args)
    {
        m_dispatching = true;
        bool hasRemoved = false;

        for (auto& e : m_callbacks)
        {
            if (e.removed)
            {
                hasRemoved = true;
                continue;
            }

            e.callback(std::forward<Args>(args)...);
        }

        m_dispatching = false;

        if (hasRemoved)
        {
            std::erase_if(m_callbacks,
               [](const Entry& e)
               {
                   return e.removed;
               });
        }
    }

} // end namespace Event 
