#include "ecs/entities/PrefabEntity.hpp"
#include "ecs/components/ComponentRegistrator.hpp"


ecs::PrefabEntity::~PrefabEntity()
{
    clear();
}

void ecs::PrefabEntity::clear()
{
    for (componentId_t componentId = 0; componentId < m_dataByComponentsIndex.size(); ++componentId)
    {
        if (m_dataByComponentsIndex[componentId] == nullptr)
            continue;

        ComponentRegistrator::GetInfo(componentId).destructor(m_dataByComponentsIndex[componentId].get());
    }
    m_dataByComponentsIndex.clear();
}

