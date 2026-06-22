#pragma once
#include "../PrefabEntity.hpp"


inline ecs::PrefabEntity::~PrefabEntity() { clear(); }

template <ecs::IsComponent ComponentCls, typename... Args>
void ecs::PrefabEntity::AddComponent(Args&&... args)
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
