#pragma once
#include <memory>
#include <stdexcept>
#include "../Listener.hpp"

namespace event
{

    template<typename T, typename... Args>
    Listener<T, Args...>::Listener(Event_t* event, const Callback_t callback, const deleter_t& deleter) :
        AbstractListener<T>(event, deleter)
        {
            addCallback(callback); 
        }

    template<typename T, typename... Args>
    Listener<T, Args...>::Listener(Event_t* event, const std::vector<Callback_t>&& callbacks, const deleter_t deleter) :
        AbstractListener<T>(event, deleter)
        {
            static_assert(!std::is_same_v<T, callbackId_t>, "T must not be equal to callbackId_t");

            if (this->m_event == nullptr)
                return;

            for (const auto callback : callbacks)
            {
                addCallback(callback);
            }
        }
    
    template<typename T, typename... Args>
    void Listener<T, Args...>::addCallback(Callback_t callback)
    {
        if (this->m_event == nullptr)
            return;

        auto* event = dynamic_cast<Event<Args...>*>(this->m_event);
        if (event != nullptr)
            this->addCallbackId(event->add(callback));
    }

    template<typename T, typename... Args>
    void Listener<T, Args...>::swap(Listener && other)
    {
        std::swap(this->m_callbackId, other.m_callbackId);
        std::swap(this->m_event, other.m_event);
    }

} // end namespace Event 
