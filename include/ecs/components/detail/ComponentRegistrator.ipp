#pragma once
#include <stdexcept>
#include "../ComponentRegistrator.hpp"


template<typename ComponentCls>
ecs::RegisterComponentInfo ecs::RegisterComponentInfo::Create(const std::string& name)
{
    return {.name = std::string(name),
            .componentSize = sizeof(ComponentCls),
            .componentId = INVALID_COMPONENT_ID,
            .constructor = [](byte* ptr) { new(ptr) ComponentCls(); },
            .destructor = [](byte* ptr) { reinterpret_cast<ComponentCls*>(ptr)->~ComponentCls(); },
            .copy = [](byte* to, byte* from) { new(to) ComponentCls(reinterpret_cast<const ComponentCls&>(*from)); }
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
        throw std::out_of_range("Invalid component id");
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
