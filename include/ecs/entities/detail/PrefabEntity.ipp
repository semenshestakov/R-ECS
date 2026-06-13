#pragma once
#include "../PrefabEntity.hpp"


template<ecs::IsComponent ComponentCls, typename... Args>
void ecs::PrefabEntity::AddComponent(Args&&... args)
{
    static const componentId_t componentId = ComponentRegistrator::GetComponentId<ComponentCls>();
    if(m_dataByComponentsIndex.size() <= componentId)
    {
        m_dataByComponentsIndex.resize(componentId + 1);
    }

    byte* raw = ComponentFreeList<ComponentCls>::acquire();
    new(raw) ComponentCls(std::forward<Args>(args)...);

    if (m_dataByComponentsIndex[componentId] == nullptr)
        m_isDirtyArchetype = true;

    m_archetype.set(componentId);
    m_dataByComponentsIndex[componentId] = { raw, PoolDeleter{ componentId } };
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