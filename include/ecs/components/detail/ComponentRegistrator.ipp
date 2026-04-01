#pragma once
#include "../ComponentRegistrator.hpp"


template<typename ComponentCls>
ecs::RegisterComponentInfo ecs::RegisterComponentInfo::Create(const std::string& name)
{
    return {.name = std::string(name),
            .componentSize = sizeof(ComponentCls),
            .componentId = ComponentCls::componentId,
            .constructor = [](byte* ptr) { new(ptr) ComponentCls(); },
            .destructor = [](byte* ptr) { reinterpret_cast<ComponentCls*>(ptr)->~ComponentCls(); }};
}

template<typename ComponentCls>
ecs::componentId_t ecs::ComponentRegistrator::Register(const std::string& name)
{
    Super registrator = Super::Create<ComponentCls>(name);

    const std::size_t& index = Super::getIndex(registrator);
    Super::setIndex(registrator, Super::INVALID_INDEX);

    const auto componentId = static_cast<componentId_t>(index + 1);
    Super::s_collection[index]->componentId = componentId;
    return componentId;
}

/* static */ inline const ecs::RegisterComponentInfo& ecs::ComponentRegistrator::GetInfo(const componentId_t componentId)
{
    if (componentId == INVALID_COMPONENT_ID)
        throw std::out_of_range("Invalid component id");
    return Super::s_collection.at(static_cast<std::size_t>(componentId - 1)).value();
}
