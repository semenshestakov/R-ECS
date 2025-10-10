#pragma once

#include "../StaticComponents.hpp"
#include "../ComponentError.hpp"


namespace ecs::component
{

    /* private */ inline StaticComponents::StaticComponents(const ComponentsInfo &componentsInfo)
        : m_componentsInfo(componentsInfo)
    {

    }
    /* public */ inline StaticComponents::StaticComponents() = default;

    /* public */ inline StaticComponents::~StaticComponents()
    {
        if (m_componentsInfo.maxComponentId == INVALID_COMPONENT_ID)
            return;

        for (const auto * componentInfo = beginComponentInfo(); componentInfo != endComponentInfo(); ++componentInfo)
        {
            if (componentInfo->initialized)
                reinterpret_cast<BaseComponent*>(componentInfo->ptr)->~BaseComponent();
        }
    }

    /* public */ inline void StaticComponents::initialize()
    {
        if (m_componentsInfo.maxComponentId == INVALID_COMPONENT_ID)
            return;

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

    template<BaseOfComponents COMPONENT, typename... Args>
    /* public */ bool StaticComponents::init(const Args &... args)
    {
        if (m_componentsInfo.maxComponentId == INVALID_COMPONENT_ID)
            return false;

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
    const COMPONENT *StaticComponents::get() const
    {
        if (m_componentsInfo.maxComponentId == INVALID_COMPONENT_ID)
            return nullptr;

        const ComponentInfo* componentInfo = getComponentInfoByComponent<COMPONENT>();
        if (componentInfo == nullptr)
            return nullptr;

        if (sizeof(COMPONENT) > componentInfo->componentSize() || !componentInfo->initialized)
            return nullptr;

        return reinterpret_cast<COMPONENT*>(componentInfo->ptr);
    }

    template<BaseOfComponents COMPONENT>
    /* private */ StaticComponents::ComponentInfo* StaticComponents::getComponentInfoByComponent() const
    {
        return getComponentInfoByComponentId(COMPONENT::componentId);
    }

    /* private */ inline StaticComponents::ComponentInfo* StaticComponents::getComponentInfoByComponentId(const componentId_t componentId) const
    {
        if (componentId > m_componentsInfo.maxComponentId || componentId == INVALID_COMPONENT_ID)
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

    /* private */ inline StaticComponents::ComponentInfo *StaticComponents::beginComponentInfo() const
    {
        return reinterpret_cast<ComponentInfo*>(data() + sizeof(StaticComponents));
    }

    /* private */ inline StaticComponents::ComponentInfo *StaticComponents::endComponentInfo() const
    {
        return beginComponentInfo() + m_componentsInfo.maxComponentId + 1;
    }

    /* private */ inline byte* StaticComponents::data() const
    {
        return const_cast<byte*>(reinterpret_cast<const byte*>(this));
    }

    /* static */inline byte* StaticComponents::newBuffer(const bufferSize_t classBufferSize, const componentId_t maxComponentId)
    {
        return new byte[sizeof(StaticComponents) + (maxComponentId + 1) * sizeof(ComponentInfo) + classBufferSize]{};
    }

} // namespace ecs::component
