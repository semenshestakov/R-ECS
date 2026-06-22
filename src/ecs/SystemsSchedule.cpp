#include "ecs/systems/SystemsSchedule.hpp"



ecs::SystemsSchedule::SystemsSchedule() = default;

ecs::SystemsSchedule::~SystemsSchedule() = default;

void ecs::SystemsSchedule::Add(const IBaseSystem& system, const systemHash_t hash)
{
    const DependentSystems dependentSystems = system.GetDependents();
    if (dependentSystems.count != 0)
    {
        for (std::size_t i = 0; i < dependentSystems.count; ++i)
        {
            m_graph.emplace(hash, *(dependentSystems.systemHashes + i));
        }
    }
    else
    {
        m_graph.emplace(hash);
    }

    if (m_isInit)
    {
        m_stagesGraph = m_graph.build();
    }
}

void ecs::SystemsSchedule::Init()
{
    if (!m_isInit)
    {
        m_stagesGraph = m_graph.build();
        m_isInit = true;
    }
}

void ecs::SystemsSchedule::clear()
{
    m_graph.clear();
    m_stagesGraph.clear();
}
