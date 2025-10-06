#pragma once

#include "../StaticComponents.hpp"
#include "../ComponentError.hpp"


namespace ecs::component
{

    inline StaticComponents::StaticComponents() = default;

    inline StaticComponents::StaticComponents(const bufferSize_t classBufferSize, const componentId_t maxComponentId)
    {
        m_buffer = newBuffer(classBufferSize, maxComponentId);
        m_maxComponentId = maxComponentId;
    }

    inline StaticComponents::StaticComponents(byte *buffer, const componentId_t maxComponentId)
    {
        m_buffer = buffer;
        m_maxComponentId = maxComponentId;
    }

    inline StaticComponents::~StaticComponents()
    {
        if (m_buffer == nullptr)
            return;

        for (const auto * componentInfo = beginComponentInfo(); componentInfo < endComponentInfo(); ++componentInfo)
        {
            if (componentInfo->initialized)
                reinterpret_cast<BaseComponent*>(componentInfo->ptr)->~BaseComponent();
        }

        delete[] m_buffer;
        m_buffer = nullptr;
    }

    inline StaticComponents::StaticComponents(StaticComponents &&other) noexcept
    {
        this->swap(other);
    }
    inline StaticComponents& StaticComponents::operator=(StaticComponents &&other) noexcept
    {
        this->swap(other);
        return *this;
    }

    inline void StaticComponents::initialize()
    {
        for (auto * componentInfo = beginComponentInfo(); componentInfo != endComponentInfo(); ++componentInfo)
        {
            if (!componentInfo->initialized
                && componentInfo->ptr != nullptr
                && componentInfo->registerComponentInfo->constructor != nullptr
                )
            {
                componentInfo->registerComponentInfo->constructor(componentInfo->ptr);
                componentInfo->initialized = true;
            }
        }
    }

    inline StaticComponents::ComponentInfo* StaticComponents::getComponentInfoByComponentId(const componentId_t componentId) const
    {
        if (m_buffer == nullptr)
            return nullptr;

        if (componentId > m_maxComponentId || componentId == INVALID_COMPONENT_ID)
            return nullptr;

        ComponentInfo* componentInfo = beginComponentInfo() + componentId;
        if (const componentId_t findComponentId = componentInfo->componentId(); findComponentId == componentId)
        {
            if (findComponentId != INVALID_COMPONENT_ID && findComponentId != componentId)
                throw error::InvalidComponentId("findComponentId: %u, componentId: %u", findComponentId, componentId);

            return componentInfo;
        }

        return nullptr;
    }

    template<BaseOfComponents COMPONENT, typename... Args>
    bool StaticComponents::init(const Args &... args)
    {
        ComponentInfo* componentInfo = getComponentInfoByComponent<COMPONENT>();
        if (componentInfo == nullptr)
            return false;

        if (componentInfo->componentSize() < sizeof(COMPONENT))
            throw error::InvalidSizeComponents(
                "[StaticComponents::init] [%s] componentInfo->componentSize: %u, sizeof(COMPONENT): %u",
                typeid(COMPONENT).name(), componentInfo->componentSize(), sizeof(COMPONENT)
                );

        for (bufferSize_t i = 0; i < componentInfo->componentSize(); ++i)
            componentInfo->ptr[i] = 0;

        if (componentInfo->initialized)
            reinterpret_cast<BaseComponent*>(componentInfo->ptr)->~BaseComponent();

        componentInfo->initialized = true;
        new (componentInfo->ptr) COMPONENT(args...);
        return true;
    }

    template<BaseOfComponents COMPONENT>
    COMPONENT* StaticComponents::get() const
    {
        const ComponentInfo* componentInfo = getComponentInfoByComponent<COMPONENT>();
        if (componentInfo == nullptr)
            return nullptr;

        if (sizeof(COMPONENT) > componentInfo->componentSize() || !componentInfo->initialized)
            return nullptr;

        return reinterpret_cast<COMPONENT*>(componentInfo->ptr);
    }

    template<BaseOfComponents COMPONENT>
    StaticComponents::ComponentInfo* StaticComponents::getComponentInfoByComponent() const
    {
        return getComponentInfoByComponentId(COMPONENT::componentId);
    }

    inline byte *StaticComponents::getComponentsDataPtr() const
    {
        if (m_buffer == nullptr)
            return nullptr;

        return m_buffer + m_maxComponentId * sizeof(ComponentInfo);
    }

    inline StaticComponents::ComponentInfo *StaticComponents::beginComponentInfo() const
    {
        return reinterpret_cast<ComponentInfo*>(m_buffer);
    }

    inline StaticComponents::ComponentInfo *StaticComponents::endComponentInfo() const
    {
        return reinterpret_cast<ComponentInfo*>(m_buffer) + m_maxComponentId + 1;
    }

    inline void StaticComponents::swap(StaticComponents &other) noexcept
    {
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_maxComponentId, other.m_maxComponentId);
    }

    /* static */inline byte *StaticComponents::newBuffer(const bufferSize_t classBufferSize, const componentId_t maxComponentId)
    {
        return new byte[(maxComponentId + 1) * sizeof(ComponentInfo) + classBufferSize];
    }

} // namespace ecs::component
