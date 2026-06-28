#pragma once
#include "../PrefabEntity.hpp"


inline ecs::PrefabEntity::~PrefabEntity() { clear(); }

template <ecs::IsComponent ComponentCls, typename... Args>
ComponentCls& ecs::PrefabEntity::AddComponent(Args&&... args)
{
    static const componentId_t componentId = ComponentRegistrator::GetComponentId<ComponentCls>();
    if (m_dataByComponentsIndex.size() <= componentId)
    {
        m_dataByComponentsIndex.resize(componentId + 1);
    }

    byte* raw = ComponentFreeList<ComponentCls>::acquire();
    new (raw) ComponentCls(std::forward<Args>(args)...);

    if (m_dataByComponentsIndex[componentId] == nullptr)
        m_isDirtyArchetype = true;

    m_archetype.set(componentId);
    m_dataByComponentsIndex[componentId] = {raw, PoolDeleter{componentId}};
    return GetComponent<ComponentCls>();
}

template <ecs::IsComponent ComponentCls>
ComponentCls* ecs::PrefabEntity::TryGetComponent()
{
    static const componentId_t componentId = ComponentRegistrator::GetComponentId<ComponentCls>();
    if (componentId < m_dataByComponentsIndex.size() && m_dataByComponentsIndex[componentId] != nullptr)
        return std::bit_cast<ComponentCls*>(m_dataByComponentsIndex[componentId].get());

    return nullptr;
}

template <ecs::IsComponent ComponentCls>
const ComponentCls* ecs::PrefabEntity::TryGetComponent() const
{
    static const componentId_t componentId = ComponentRegistrator::GetComponentId<ComponentCls>();
    if (componentId < m_dataByComponentsIndex.size() && m_dataByComponentsIndex[componentId] != nullptr)
        return std::bit_cast<ComponentCls*>(m_dataByComponentsIndex[componentId].get());

    return nullptr;
}

template <ecs::IsComponent ComponentCls>
ComponentCls& ecs::PrefabEntity::GetComponent()
{
    auto* component = TryGetComponent<ComponentCls>();
    assert(component != nullptr);
    return *component;
}

template <ecs::IsComponent ComponentCls>
const ComponentCls& ecs::PrefabEntity::GetComponent() const
{
    const auto* component = TryGetComponent<ComponentCls>();
    assert(component != nullptr);
    return *component;
}

inline void ecs::PrefabEntity::PoolDeleter::operator()(byte* ptr) const
{
    ComponentRegistrator::GetInfo(componentId).poolRelease(ptr);
}

inline void ecs::PrefabEntity::clear()
{
    for (componentId_t componentId = 0; componentId < m_dataByComponentsIndex.size(); ++componentId)
    {
        if (m_dataByComponentsIndex[componentId] == nullptr)
            continue;

        ComponentRegistrator::GetInfo(componentId).destructor(m_dataByComponentsIndex[componentId].get());
    }
    m_dataByComponentsIndex.clear();
}

inline const ecs::Archetype& ecs::PrefabEntity::getArchetype() const
{
    if (m_isDirtyArchetype)
    {
        m_archetype.updateHash();
        m_isDirtyArchetype = false;
    }

    return m_archetype;
}
