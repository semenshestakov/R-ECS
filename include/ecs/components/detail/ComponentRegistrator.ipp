#pragma once
#include <bit>
#include <stdexcept>
#include "../ComponentRegistrator.hpp"
#include "ecs/utils/ComponentError.hpp"


template<typename ComponentCls>
ecs::RegisterComponentInfo ecs::RegisterComponentInfo::Create(const std::string& name)
{
    return {.name = std::string(name),
            .componentSize = sizeof(ComponentCls),
            .componentId = INVALID_COMPONENT_ID,
            .constructor = [](byte* ptr) { new(ptr) ComponentCls(); },
            .destructor = [](byte* ptr) { std::bit_cast<ComponentCls*>(ptr)->~ComponentCls(); },
            .copy = [](byte* to, byte* from) { new(to) ComponentCls(*std::bit_cast<const ComponentCls*>(from)); },
            .move = [](byte* to, byte* from) { new(to) ComponentCls(std::move(*std::bit_cast<ComponentCls*>(from)));}
    };
}

template<typename ComponentCls>
ecs::componentId_t ecs::ComponentRegistrator::Register()
{
    Super registrator = Super::Create<ComponentCls>(typeid(ComponentCls).name());

    const std::size_t& index = registrator.getIndex();
    Super::setIndex(registrator, Super::INVALID_INDEX);

    const auto componentId = static_cast<componentId_t>(index + 1);
    Super::s_collection[index].second->componentId = componentId;
    return componentId;
}

/* static */ inline const ecs::RegisterComponentInfo& ecs::ComponentRegistrator::GetInfo(const componentId_t componentId)
{
    if(componentId == INVALID_COMPONENT_ID)
        throw error::InvalidComponentId("ComponentRegistrator::GetInfo; componentId == INVALID_COMPONENT_ID");
    return Super::s_collection.at(static_cast<std::size_t>(componentId - 1)).second.value();
}

template<typename ComponentCls>
/* static */ ecs::componentId_t ecs::ComponentRegistrator::GetСomponentId()
{
    static componentId_t s_componentId = []()
    {
        return ComponentRegistrator::Register<ComponentCls>();
    }();
    return s_componentId;
}
