#pragma once
#include <cassert>
#include "../EntityWrapper.hpp"
#include "ecs/components/ComponentRegistrator.hpp"


template<ecs::IsComponent ComponentCls>
ComponentCls& ecs::EntityWrapper::GetComponent()
{
    ComponentCls* componentPtr = TryGetComponent<ComponentCls>();
    assert(componentPtr != nullptr);
    return *componentPtr;
}

template<ecs::IsComponent ComponentCls>
const ComponentCls& ecs::EntityWrapper::GetComponent() const
{
    const ComponentCls* componentPtr = TryGetComponent<ComponentCls>();
    assert(componentPtr != nullptr);
    return *componentPtr;
}

template<ecs::IsComponent ComponentCls>
ComponentCls* ecs::EntityWrapper::TryGetComponent()
{
    return std::bit_cast<ComponentCls*>(GetComponentData(ComponentRegistrator::GetСomponentId<ComponentCls>()));
}

template<ecs::IsComponent ComponentCls>
const ComponentCls* ecs::EntityWrapper::TryGetComponent() const
{
    return std::bit_cast<const ComponentCls*>(GetComponentData(ComponentRegistrator::GetСomponentId<ComponentCls>()));
}
