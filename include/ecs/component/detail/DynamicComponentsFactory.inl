#pragma once
#include <future>
#include "../ComponentError.hpp"
#include "../DynamicComponentsFactory.hpp"


namespace ecs::component
{

    inline DynamicComponentsFactory::DynamicComponentsFactory() :
        DynamicComponentsFactory(0)
    {

    }

    inline DynamicComponentsFactory::DynamicComponentsFactory(const bufferSize_t bufferSize) :
        m_bufferCapacity(bufferSize)
    {
        if (m_bufferCapacity != 0)
            m_buffer = new byte[bufferSize > s_sizeOfStackHead ? bufferSize : (s_sizeOfStackHead * 2)](0);
    }


    inline DynamicComponentsFactory::~DynamicComponentsFactory() noexcept
    {
        if (m_bufferCapacity == 0)
            return;

        auto* headByte = m_buffer;
        auto* head = reinterpret_cast<StackHead*>(headByte);

        while(head != nullptr && headByte < m_buffer + m_bufferCapacity)
        {
            if (head->componentTypeId != INVALID_COMPONENT_ID)
            {
                reinterpret_cast<BaseComponent*>(headByte + s_sizeOfStackHead)->~BaseComponent();
            }
            headByte = reinterpret_cast<byte*>(head->next());
            head = head->next();
        }

        delete[] m_buffer;
        m_buffer = nullptr;
    }

    inline DynamicComponentsFactory::DynamicComponentsFactory(DynamicComponentsFactory&& other) noexcept
    {
        this->swap(other);
    }

    inline DynamicComponentsFactory& DynamicComponentsFactory::operator=(DynamicComponentsFactory&& other) noexcept
    {
        this->swap(other);
        return *this;
    }

    
    inline void DynamicComponentsFactory::swap(DynamicComponentsFactory& other) noexcept
    {
        std::swap(m_bufferCapacity, other.m_bufferCapacity);
        std::swap(m_buffer, other.m_buffer);
    }


    template <BaseOfComponents COMPONENT, typename... Args>
    void DynamicComponentsFactory::add(Args&& ... args)
    {
        static constexpr bufferSize_t sizeOfClass = sizeof(COMPONENT);
        static constexpr componentId_t componentId = getComponentsTypeId<COMPONENT>();

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

        auto* instance = new (reinterpret_cast<byte*>(currentHead) + s_sizeOfStackHead) COMPONENT(std::forward<Args>(args)...);

        currentHead->componentTypeId = componentId;
        currentHead->componentSize = sizeOfClass;

        new (reinterpret_cast<byte*>(instance) + sizeOfClass) StackHead {};
    }

    template <BaseOfComponents COMPONENT>
    COMPONENT* DynamicComponentsFactory::get()
    {
        static constexpr componentId_t componentId = getComponentsTypeId<COMPONENT>();

        StackHead* finedHead = findStackHeadById(componentId);
        if (finedHead == nullptr)
            return nullptr;

        if (finedHead->componentSize < sizeof(COMPONENT))
            throw ComponentError(
                "[get] finedHead->componentSize < sizeof(COMPONENT); finedHead.componentTypeId: %u, componentId: %u, className: %s",
                finedHead->componentTypeId, componentId, typeid(COMPONENT).name()
                );

        return reinterpret_cast<COMPONENT*>(reinterpret_cast<byte*>(finedHead) + s_sizeOfStackHead);
    }

    // utils
    inline bool DynamicComponentsFactory::resize(bufferSize_t a_size) noexcept
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

    inline DynamicComponentsFactory::StackHead* DynamicComponentsFactory::getLastHead() const noexcept
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

    inline DynamicComponentsFactory::StackHead* DynamicComponentsFactory::findStackHeadById(const componentId_t componentId) const noexcept
    {
        if (m_bufferCapacity == 0 || componentId == 0)
            return nullptr;

        byte* byteHead = m_buffer;
        StackHead* stackHead = reinterpret_cast<StackHead*>(byteHead);

        while ((byteHead + s_sizeOfStackHead) < (m_buffer + m_bufferCapacity))
        {
            if (stackHead->componentTypeId == componentId)
                return stackHead;

            if (stackHead->componentSize == 0)
                return nullptr;

            stackHead = stackHead->next();
            byteHead = reinterpret_cast<byte*>(stackHead);
        }

        return nullptr;
    }

    template <BaseOfComponents COMPONENT>
    constexpr /* static */ componentId_t DynamicComponentsFactory::getComponentsTypeId()
    {
        static_assert(std::is_base_of_v<BaseComponent, COMPONENT>);
        static_assert(COMPONENT::componentId != INVALID_COMPONENT_ID);
        return COMPONENT::componentId;
    }

}