#pragma once
#include <bit>
#include <mutex>

#include "../ComponentFreeList.hpp"
#include "../ComponentRegistrator.hpp"
#include "../Utils.hpp"
#include "common_recs/utils/Demangle.hpp"


template <ecs::IsComponent ComponentCls>
ecs::RegisterComponentInfo ecs::RegisterComponentInfo::Create(const std::string& name)
{
    if constexpr (IsTag<ComponentCls>)
    {
        return {.name = std::string(name),
                .dname = demangle(name.c_str()),
                .componentSize = 0,
                .componentId = INVALID_COMPONENT_ID,
                .isTag = true};
    }
    else
    {
        return {.name = std::string(name),
                .dname = demangle(name.c_str()),
                .componentSize = sizeof(ComponentCls),
                .componentId = INVALID_COMPONENT_ID,
                .isTag = false,
                .constructor = [](byte* ptr) { new(ptr) ComponentCls(); },
                .destructor = [](byte* ptr) { std::bit_cast<ComponentCls*>(ptr)->~ComponentCls(); },
                .copy = [](byte* to, byte* from) { new(to) ComponentCls(*std::bit_cast<const ComponentCls*>(from)); },
                .move = [](byte* to, byte* from) { new(to) ComponentCls(std::move(*std::bit_cast<ComponentCls*>(from)));},
                .poolAcquire  = []() -> byte* { return ComponentFreeList<ComponentCls>::acquire(); },
                .poolRelease  = [](byte* ptr)  { ComponentFreeList<ComponentCls>::release(ptr); }
        };
    }
}

template<ecs::IsComponent ComponentCls>
ecs::componentId_t ecs::ComponentRegistrator::Register()
{
    // First-time registration mutates the shared component collection and may run
    // from a worker (a type first touched inside a parallel view). The per-type
    // magic static in GetComponentId() bounds this to once per type; the mutex
    // serializes distinct types registering concurrently.
    const std::lock_guard<std::mutex> lock(s_registrationMutex);

    Super registrator = Super::Create<ComponentCls>(typeid(ComponentCls).name());

    const std::size_t& index = registrator.getIndex();
    Super::setIndex(registrator, Super::INVALID_INDEX);

    const auto componentId = static_cast<componentId_t>(index + 1);
    Super::collection()[index].second->componentId = componentId;
    return componentId;
}

/* static */ inline const ecs::RegisterComponentInfo& ecs::ComponentRegistrator::GetInfo(const componentId_t componentId)
{
#ifndef NDEBUG
    if(componentId == INVALID_COMPONENT_ID)
        throw error::InvalidComponentId("ComponentRegistrator::GetInfo; componentId == INVALID_COMPONENT_ID");
#endif
    return Super::collection().at(static_cast<std::size_t>(componentId - 1)).second.value();
}

template<ecs::IsComponent ComponentCls>
/* static */ ecs::componentId_t ecs::ComponentRegistrator::GetComponentId()
{
    static componentId_t s_componentId = []()
    {
        return ComponentRegistrator::Register<ComponentCls>();
    }();
    return s_componentId;
}
