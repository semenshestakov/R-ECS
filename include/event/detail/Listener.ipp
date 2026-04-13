#pragma once
#include "../Listener.hpp"

namespace event
{

    template<typename T, typename... Args>
    Listener<T, Args...>::Listener() :
        Listener(nullptr)
    {
    }

    template<typename T, typename... Args>
    Listener<T, Args...>::Listener(Event_t* event) :
        Listener(event, nullptr)
    {
    }

    template<typename T, typename... Args>
    Listener<T, Args...>::Listener(
        Event_t* event, const Callback_t& callback, const deleter_t& deleter /* = nullptr */
        ) :
        AbstractListener<T>(event, deleter)
    {
        subscribe(callback);
    }

    template<typename T, typename... Args>
    Listener<T, Args...>::Listener(
        Event_t* event, const std::vector<Callback_t>& callbacks, const deleter_t& deleter /* = nullptr */
        ) :
        AbstractListener<T>(event, deleter)
    {
        static_assert(!std::is_same_v<T, callbackId_t>, "T must not be equal to callbackId_t");

        if(this->m_event == nullptr)
            return;

        for(const auto callback : callbacks)
        {
            subscribe(callback);
        }
    }
    template<typename T, typename... Args>
    Listener<T, Args...>::Listener(
        Event_t* event, const std::vector<std::pair<Callback_t, int>>& callbacksWithPriority, const deleter_t& deleter /* = nullptr */
        )
    {
        static_assert(!std::is_same_v<T, callbackId_t>, "T must not be equal to callbackId_t");

        if(this->m_event == nullptr)
            return;

        for(const auto [callback, priority] : callbacksWithPriority)
        {
            subscribe(callback, priority);
        }
    }

    template<typename T, typename... Args>
    Listener<T, Args...>::Listener(Listener&& other) noexcept
    {
        this->swap(std::move(other));
    }

    template<typename T, typename... Args>
    Listener<T, Args...>& Listener<T, Args...>::operator=(Listener&& other) noexcept
    {
        this->swap(std::move(other));
        return *this;
    }

    template<typename T, typename... Args>
    void Listener<T, Args...>::swap(Listener&& other)
    {
        std::swap(this->m_callbackId, other.m_callbackId);
        std::swap(this->m_event, other.m_event);
    }

    template<typename T, typename... Args>
    void Listener<T, Args...>::subscribe(const Callback_t& callback, int priority /* = 0 */)
    {
        if(this->m_event == nullptr || callback == nullptr)
            return;

        auto* event = dynamic_cast<Event<Args...>*>(this->m_event);
        if(event != nullptr)
            this->addCallbackId(event->add(callback));
    }

    template<typename T, typename... Args>
    void Listener<T, Args...>::subscribe(Event_t* event, const Callback_t& callback, const int priority /* = 0 */)
    {
        if(this->m_event != nullptr)
        {
            this->clear();
            return;
        }

        if(event == nullptr)
            return;

        this->m_event = event;
        subscribe(callback, priority);
    }

} // end namespace Event 
