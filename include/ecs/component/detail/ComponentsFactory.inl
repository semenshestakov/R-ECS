#pragma once
#include <future>
#include "../ComponentsFactory.hpp"
#include "../ComponentError.hpp"


namespace ecs::component
{

    inline ComponentsFactory::ComponentsFactory() :
        ComponentsFactory(0)
    {

    }

    inline ComponentsFactory::ComponentsFactory(const bufferSize_t bufferSize) :
        m_bufferCapacity(bufferSize)
    {
        if (m_bufferCapacity != 0)
            m_buffer = new byte[bufferSize > s_sizeOfStackHead ? bufferSize : (s_sizeOfStackHead * 2)](0);
    }


    inline ComponentsFactory::~ComponentsFactory() noexcept
    {
        if (m_bufferCapacity == 0)
            return;

        auto* headByte = m_buffer;
        auto* head = reinterpret_cast<StackHead*>(headByte);

        while(head != nullptr && headByte < m_buffer + m_bufferCapacity)
        {
            if (head->componentTypeId != INVALID_COMPONENT_ID)
            {
                if (head->destructor)
                    head->destructor(headByte + s_sizeOfStackHead);
            }
            headByte = reinterpret_cast<byte*>(head->next());
            head = head->next();
        }

        delete[] m_buffer;
        m_buffer = nullptr;
    }

    inline ComponentsFactory::ComponentsFactory(ComponentsFactory&& a_other) noexcept
    {
        this->swap(a_other);
    }

    inline ComponentsFactory& ComponentsFactory::operator=(ComponentsFactory&& a_other) noexcept
    {
        this->swap(a_other);
        return *this;
    }

    
    inline void ComponentsFactory::swap(ComponentsFactory& a_other) noexcept
    {
        std::swap(m_bufferCapacity, a_other.m_bufferCapacity);
        std::swap(m_buffer, a_other.m_buffer);
    }


    template <class CLASS, typename... Args>
    void ComponentsFactory::add(Args&& ... args)
    {
        static constexpr bufferSize_t sizeOfClass = sizeof(CLASS);
        static constexpr componentId_t componentId = getComponentsTypeId<CLASS>();

        if (findStackHeadById(componentId) != nullptr)
            throw ComponentError("[add] findStackHeadById(componentId) != nullptr");

        // resize if need
        {
            StackHead* lastComponentHeadStack = getLastHead();
            // buffer is empty
            if (lastComponentHeadStack == nullptr)
            {
                resize((sizeOfClass + s_sizeOfStackHead) * 2);
            }
            // (new size + old size) > capacity
            else if (reinterpret_cast<byte*>(lastComponentHeadStack) + sizeOfClass + s_sizeOfStackHead * 2 > m_buffer + m_bufferCapacity)
            {
                resize(m_bufferCapacity * 2);
            }
        }

        StackHead* currentHead = nullptr;
        {
            StackHead* lastComponentHeadStack = getLastHead();
            if (lastComponentHeadStack == nullptr)
                throw ComponentError("[add] lastComponentHeadStack == nullptr");

            if (lastComponentHeadStack->componentSize == 0)  // fist add
            {
                currentHead = lastComponentHeadStack;
            }
            else // add to next
            {
                currentHead = lastComponentHeadStack->next();
            }
        }

        // validation
        if (currentHead->componentSize != 0)
            throw ComponentError("[add] componentSize(%u) != 0", currentHead->componentSize);

        if (currentHead->componentTypeId != INVALID_COMPONENT_ID)
            throw ComponentError("[add] componentTypeId(%u) != INVALID_COMPONENT_ID", currentHead->componentTypeId);

        if (reinterpret_cast<byte*>(currentHead) + (sizeOfClass + s_sizeOfStackHead * 2) > m_buffer + m_bufferCapacity)
            throw ComponentError("[add] newSize > capacity; sizeOfClass: %u, bufferCapacity: %u", sizeOfClass, m_bufferCapacity);

        auto* instance = new (reinterpret_cast<byte*>(currentHead) + s_sizeOfStackHead) CLASS(std::forward<Args>(args)...);

        currentHead->componentTypeId = componentId;
        currentHead->componentSize = sizeOfClass;
        currentHead->destructor = [](void* ptr) { static_cast<CLASS*>(ptr)->~CLASS(); };

        new (reinterpret_cast<byte*>(instance) + sizeOfClass) StackHead {};
    }

    template <class CLASS>
    CLASS* ComponentsFactory::get()
    {
        static constexpr componentId_t componentId = getComponentsTypeId<CLASS>();

        StackHead* finedHead = findStackHeadById(componentId);
        if (finedHead == nullptr)
            return nullptr;

        if (finedHead->componentSize < sizeof(CLASS))
            throw ComponentError(
                "[get] finedHead->componentSize < sizeof(CLASS); finedHead.componentTypeId: %u, componentId: %u, className: %s",
                finedHead->componentTypeId, componentId, typeid(CLASS).name()
                );

        return reinterpret_cast<CLASS*>(reinterpret_cast<byte*>(finedHead) + s_sizeOfStackHead);
    }

    // utils
    inline bool ComponentsFactory::resize(bufferSize_t a_size) noexcept
    {
        if (a_size <= m_bufferCapacity)
            return false;

        a_size = a_size > s_sizeOfStackHead ? a_size : s_sizeOfStackHead * 2;
        const auto newBuffer = new byte[a_size] ();

        memcpy(newBuffer, m_buffer, m_bufferCapacity);

        if (m_bufferCapacity != 0)
            delete[] m_buffer;

        m_buffer = newBuffer;
        m_bufferCapacity = a_size;
        return true;
    }

    inline ComponentsFactory::StackHead* ComponentsFactory::getLastHead() const noexcept
    {
        if (m_bufferCapacity == 0)
            return nullptr;

        byte* byteHead = m_buffer;
        StackHead* stackHead = reinterpret_cast<StackHead*>(byteHead);

        while (stackHead->componentSize != 0 && byteHead < (m_buffer + m_bufferCapacity))
        {
            byteHead = reinterpret_cast<byte*>(stackHead->next());
            stackHead = stackHead->next();
        }

        return stackHead;
    }

    inline ComponentsFactory::StackHead* ComponentsFactory::findStackHeadById(const componentId_t a_id) const noexcept
    {
        if (m_bufferCapacity == 0 || a_id == 0)
            return nullptr;

        byte* byteHead = m_buffer;
        StackHead* stackHead = reinterpret_cast<StackHead*>(byteHead);

        while ((byteHead + s_sizeOfStackHead) < (m_buffer + m_bufferCapacity))
        {
            if (stackHead->componentTypeId == a_id)
                return stackHead;

            if (stackHead->componentSize == 0)
                return nullptr;

            stackHead = stackHead->next();
            byteHead = reinterpret_cast<byte*>(stackHead);
        }

        return nullptr;
    }

    template <class CLASS>
    constexpr /* static */ componentId_t ComponentsFactory::getComponentsTypeId()
    {
        static_assert(std::is_base_of_v<BaseComponent, CLASS>);
        static_assert(CLASS::componentId != INVALID_COMPONENT_ID);
        return CLASS::componentId;
    }

}