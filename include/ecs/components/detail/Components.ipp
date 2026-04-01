#pragma once
#include "../Components.hpp"
#include "ecs/utils/ComponentError.hpp"


inline ecs::Components::Components(const ComponentsInfo& componentsInfo) : m_componentsInfo(componentsInfo) {}

inline ecs::Components::Components() = default;

inline ecs::Components::~Components()
{
    if(m_componentsInfo.maxComponentId == INVALID_COMPONENT_ID)
        return;

    for(const auto* componentInfo = beginComponentInfo(); componentInfo != endComponentInfo(); ++componentInfo)
    {
        if(componentInfo->initialized)
            componentInfo->registerComponentInfo->destructor(componentInfo->ptr);
    }
}

template<ecs::DerivedComponent ComponentCls, typename... Args>
bool ecs::Components::init(const Args&... args)
{
    if(m_componentsInfo.maxComponentId == INVALID_COMPONENT_ID)
        return false;

    ComponentInfo* componentInfo = getComponentInfoByComponent<ComponentCls>();
    if(componentInfo == nullptr)
        return false;

    if(componentInfo->componentSize() < sizeof(ComponentCls))
        throw error::InvalidSizeComponents(
                "[Components::init] [%s] componentInfo->componentSize: %u, sizeof(ComponentCls): %u",
                typeid(ComponentCls).name(), componentInfo->componentSize(), sizeof(ComponentCls));

    for(bufferSize_t i = 0; i < componentInfo->componentSize(); ++i)
        componentInfo->ptr[i] = 0;

    if(componentInfo->initialized)
        componentInfo->registerComponentInfo->destructor(componentInfo->ptr);

    componentInfo->initialized = true;
    new(componentInfo->ptr) ComponentCls(args...);
    return true;
}

inline void ecs::Components::initialize()
{
    if(m_componentsInfo.maxComponentId == INVALID_COMPONENT_ID)
        return;

    for(auto* componentInfo = beginComponentInfo(); componentInfo != endComponentInfo(); ++componentInfo)
    {
        if(!componentInfo->initialized && componentInfo->ptr != nullptr &&
           componentInfo->registerComponentInfo->constructor != nullptr)
        {
            componentInfo->registerComponentInfo->constructor(componentInfo->ptr);
            componentInfo->initialized = true;
        }
    }
}

template<ecs::DerivedComponent ComponentCls>
ComponentCls* ecs::Components::get() const
{
    if(m_componentsInfo.maxComponentId == INVALID_COMPONENT_ID)
        return nullptr;

    const ComponentInfo* componentInfo = getComponentInfoByComponent<ComponentCls>();
    if(componentInfo == nullptr)
        return nullptr;

    if(sizeof(ComponentCls) > componentInfo->componentSize() || !componentInfo->initialized)
        return nullptr;

    return reinterpret_cast<ComponentCls*>(componentInfo->ptr);
}

template<ecs::DerivedComponent ComponentCls>
ComponentCls& ecs::Components::mustGet() const
{
    ComponentCls* component = get<ComponentCls>();
    if(component == nullptr)
        throw error::InvalidComponent("component is not found: %s", typeid(ComponentCls).name());

    return *component;
}

template<ecs::DerivedComponent... ComponentCls>
bool ecs::Components::contains() const
{
    return ((this->get<ComponentCls>() != nullptr) && ...);
}

template<ecs::DerivedComponent... ComponentCls>
std::tuple<ComponentCls&...> ecs::Components::view() const
{
    if(!this->contains<ComponentCls...>())
        throw error::InvalidComponent("view is not found");

    return std::tuple<ComponentCls&...>(*get<ComponentCls>()...);
}

inline ecs::Components::ComponentInfo*
ecs::Components::getComponentInfoByComponentId(const componentId_t componentId) const
{
    if(componentId > m_componentsInfo.maxComponentId || componentId == INVALID_COMPONENT_ID)
        return nullptr;

    ComponentInfo* componentInfo = beginComponentInfo() + componentId;
    if(const componentId_t findComponentId = componentInfo->componentId(); findComponentId == componentId)
    {
        if(findComponentId != INVALID_COMPONENT_ID && findComponentId != componentId)
            throw error::InvalidComponentId("findComponentId: %u, componentId: %u", findComponentId, componentId);

        return componentInfo;
    }

    return nullptr;
}

template<ecs::DerivedComponent ComponentCls>
ecs::Components::ComponentInfo* ecs::Components::getComponentInfoByComponent() const
{
    return getComponentInfoByComponentId(ComponentCls::componentId);
}

inline ecs::Components::ComponentInfo* ecs::Components::beginComponentInfo() const
{
    return reinterpret_cast<ComponentInfo*>(data() + sizeof(Components));
}

inline ecs::Components::ComponentInfo* ecs::Components::endComponentInfo() const
{
    return beginComponentInfo() + m_componentsInfo.maxComponentId + 1;
}

inline ecs::byte* ecs::Components::data() const { return const_cast<byte*>(reinterpret_cast<const byte*>(this)); }

inline ecs::byte* ecs::Components::newBuffer(const bufferSize_t classBufferSize, const componentId_t maxComponentId)
{
    return new byte[sizeof(Components) + (maxComponentId + 1) * sizeof(ComponentInfo) + classBufferSize]{};
}
