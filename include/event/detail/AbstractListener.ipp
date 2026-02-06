#pragma once
#include "../AbstractListener.hpp"

namespace event
{

    template <ValidCallbackIdType T>
    AbstractListener<T>::~AbstractListener()
    {
        if constexpr (std::is_same_v<T, callbackId_t>)
        {
            const callbackId_t callbackId = m_callbackId;
            removeCallbackId(m_callbackId);

            if (m_deleter != nullptr)
            {
                m_deleter(callbackId);
            }
        }
        else
        {
            T callbackIds = m_callbackId;
            for (const callbackId_t callbackId : callbackIds)
            {
                if (m_event != nullptr)
                {
                    m_event->remove(callbackId);
                }
                if (m_deleter != nullptr)
                {
                    m_deleter(callbackId);
                }
            }
        }
    }

    template <ValidCallbackIdType T>
    void AbstractListener<T>::unionCallbacks(AbstractListener&& other)
    {
        if (this->m_event->id != other.m_event->id)
        {
            throw std::runtime_error("unionCallbacks; eq eventId");
        }

        if constexpr (std::is_same_v<T, callbackId_t>)
        {
            throw std::runtime_error("is same callbackId_t");
        }
        else
        {
            for (callbackId_t callbackId : other.m_callbackId)
            {
                addCallbackId(callbackId);
            }
            other.m_callbackId.clear();
        }
    }

    template <ValidCallbackIdType T>
    eventId_t AbstractListener<T>::eventId() const
    {
        if (m_event == nullptr)
            return INVALID_EVENT_ID;

        return m_event->id;
    }

    template <ValidCallbackIdType T>
    void AbstractListener<T>::removeCallbackId(const callbackId_t callbackId)
    {
        if constexpr (std::is_same_v<T, callbackId_t>)
        {
            if (m_event != nullptr)
                m_event->remove(m_callbackId);
        }
        else
        {
            typename T::iterator it;
            if constexpr (requires { m_callbackId.find(callbackId); })
                it = m_callbackId.find(callbackId);
            else
                it = std::find(m_callbackId.begin(), m_callbackId.end(), m_callbackId);

            if (it != m_callbackId.end())
                m_callbackId.erase(it);
        }
    }

    template <ValidCallbackIdType T>
    void AbstractListener<T>::addCallbackId(const callbackId_t callbackId)
    {
        if constexpr (std::is_same_v<T, callbackId_t>)
        {
            if (m_event != nullptr)
                m_event->remove(m_callbackId);

            m_callbackId = callbackId;
        }
        else if constexpr (requires { m_callbackId.push_back(callbackId); })
        {
            m_callbackId.push_back(callbackId);
        }
        else if constexpr (requires { m_callbackId.insert(callbackId); })
        {
            m_callbackId.insert(callbackId);
        }
    }

}
