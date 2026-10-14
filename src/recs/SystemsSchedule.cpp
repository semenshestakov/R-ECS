#include "ecs/systems/SystemsSchedule.hpp"

#include <queue>


ecs::SystemsSchedule::SystemsSchedule() = default;

ecs::SystemsSchedule::~SystemsSchedule() = default;

void ecs::SystemsSchedule::Add(const IBaseSystem& system, const systemHash_t hash)
{
    m_graph.emplace(hash);

    const DependentSystems dependents = system.GetDependents();
    for (std::size_t i = 0; i < dependents.count; ++i)
        addEdge(hash, *(dependents.systemHashes + i), SystemDep::Direct);

    const DependentSystems weakDependents = system.GetWeakDependents();
    for (std::size_t i = 0; i < weakDependents.count; ++i)
        addEdge(hash, *(weakDependents.systemHashes + i), SystemDep::Data);

    const ComponentAccess access = system.GetComponentAccess();
    if (access.count != 0)
        m_access[hash].assign(access.entries, access.entries + access.count);

    m_dirty = true;
}

void ecs::SystemsSchedule::Init() { Build(); }

void ecs::SystemsSchedule::Build()
{
    if (!m_isInit || m_dirty)
    {
        deriveDataEdges();
        m_stagesGraph = m_graph.build();
        m_isInit = true;
        m_dirty = false;
    }
}

void ecs::SystemsSchedule::clear()
{
    m_graph.clear();
    m_stagesGraph.clear();
    m_access.clear();
    m_isInit = false;
    m_dirty = true;
}

std::unordered_set<ecs::systemHash_t>
ecs::SystemsSchedule::CollectHardDependents(const std::unordered_set<systemHash_t>& roots) const
{
    std::unordered_map<systemHash_t, std::vector<systemHash_t>> reverse;
    for (const auto& [node, nodeData] : m_graph)
        for (const auto& [dep, flags] : nodeData.edges())
            if (flags & SystemDep::Direct)
                reverse[dep].push_back(node);

    std::unordered_set<systemHash_t> result;
    std::queue<systemHash_t> ready;
    for (const systemHash_t root : roots)
        ready.push(root);

    while (!ready.empty())
    {
        const systemHash_t current = ready.front();
        ready.pop();

        const auto it = reverse.find(current);
        if (it == reverse.end())
            continue;

        for (const systemHash_t dependent : it->second)
            if (result.insert(dependent).second)
                ready.push(dependent);
    }

    return result;
}

void ecs::SystemsSchedule::addEdge(const systemHash_t node, const systemHash_t dep, const systemDepFlags_t flags)
{
    if (node == dep)
        return;

    const systemDepFlags_t* existing = m_graph.edge(node, dep);
    const auto merged = static_cast<systemDepFlags_t>((existing ? *existing : SystemDep::None) | flags);
    m_graph.emplace(node, dep, merged);
}

void ecs::SystemsSchedule::deriveDataEdges()
{
    struct AccessFlags
    {
        bool reads = false;
        bool writes = false;
    };

    std::unordered_map<componentHash_t, std::unordered_map<systemHash_t, AccessFlags>> byComponent;
    for (const auto& [system, entries] : m_access)
    {
        for (const auto& entry : entries)
        {
            AccessFlags& flags = byComponent[entry.component][system];
            flags.reads = flags.reads || entry.kind == AccessKind::Read;
            flags.writes = flags.writes || entry.kind == AccessKind::Write;
        }
    }

    for (const auto& [component, users] : byComponent)
    {
        std::vector<systemHash_t> writers;
        std::vector<systemHash_t> pureReaders;
        for (const auto& [system, flags] : users)
        {
            if (flags.writes)
                writers.push_back(system);
            else if (flags.reads)
                pureReaders.push_back(system);
        }

        // Readers run after every writer of the same component.
        for (const systemHash_t writer : writers)
            for (const systemHash_t reader : pureReaders)
                addEdge(reader, writer, SystemDep::Data);

        // Two writers are ordered deterministically: the smaller hash runs first.
        for (std::size_t i = 0; i < writers.size(); ++i)
            for (std::size_t j = i + 1; j < writers.size(); ++j)
            {
                const systemHash_t a = writers[i];
                const systemHash_t b = writers[j];
                if (a < b)
                    addEdge(b, a, SystemDep::Data);
                else
                    addEdge(a, b, SystemDep::Data);
            }
    }
}
