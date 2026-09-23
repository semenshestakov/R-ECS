#pragma once
#include <queue>
#include <stdexcept>
#include <unordered_set>
#include "../DirectedAcyclicGraph.hpp"


template<typename NodeType, typename EdgeData>
void collections::DirectedAcyclicGraph<NodeType, EdgeData>::emplace(const NodeType& nodeData)
{
    if(!m_nodes.contains(nodeData))
        m_nodes[nodeData] = Node(nodeData);
}

template<typename NodeType, typename EdgeData>
void collections::DirectedAcyclicGraph<NodeType, EdgeData>::emplace(const NodeType& nodeData, const NodeType& dep)
{
    // Ensure the node exists
    if(!m_nodes.contains(nodeData))
        m_nodes[nodeData] = Node(nodeData);

    // Ensure the dependency exists
    if(!m_nodes.contains(dep))
        m_nodes[dep] = Node(dep);

    // Add the dependency, keeping any existing edge payload
    m_nodes[nodeData].dependencies.try_emplace(dep);
}

template<typename NodeType, typename EdgeData>
void collections::DirectedAcyclicGraph<NodeType, EdgeData>::emplace(const NodeType& nodeData, const NodeType& dep, const EdgeData& edge)
{
    // Ensure the node exists
    if(!m_nodes.contains(nodeData))
        m_nodes[nodeData] = Node(nodeData);

    // Ensure the dependency exists
    if(!m_nodes.contains(dep))
        m_nodes[dep] = Node(dep);

    // Add the dependency, overwriting any existing edge payload
    m_nodes[nodeData].dependencies.insert_or_assign(dep, edge);
}

template<typename NodeType, typename EdgeData>
const EdgeData* collections::DirectedAcyclicGraph<NodeType, EdgeData>::edge(const NodeType& nodeData, const NodeType& dep) const
{
    const auto nodeIt = m_nodes.find(nodeData);
    if(nodeIt == m_nodes.end())
        return nullptr;

    const auto& deps = nodeIt->second.dependencies;
    const auto depIt = deps.find(dep);
    return depIt == deps.end() ? nullptr : &depIt->second;
}

template<typename NodeType, typename EdgeData>
std::vector<std::vector<NodeType>> collections::DirectedAcyclicGraph<NodeType, EdgeData>::build() const
{
    std::unordered_map<NodeType, int> inDegree;

    for(auto const& [node, _]: m_nodes)
        inDegree[node] = 0;

    for(auto const& [node, n]: m_nodes)
        for(auto const& [dep, edge]: n.dependencies)
            ++inDegree[node];

    std::queue<NodeType> ready;
    for(auto const& [node, deg]: inDegree)
        if(deg == 0)
            ready.push(node);

    std::unordered_set<NodeType> visited;
    std::vector<std::vector<NodeType>> stages;

    while(!ready.empty())
    {
        std::vector<NodeType> stage;
        size_t n = ready.size();

        for(size_t i = 0; i < n; i++)
        {
            auto current = ready.front();
            ready.pop();
            visited.insert(current);
            stage.push_back(current);

            for(auto const& [node, nodeObj]: m_nodes)
            {
                if(nodeObj.dependencies.count(current))
                {
                    --inDegree[node];
                    if(inDegree[node] == 0 && visited.count(node) == 0)
                        ready.push(node);
                }
            }
        }

        stages.push_back(stage);
    }

    if(visited.size() != m_nodes.size())
        throw std::runtime_error("Cycle detected in DAG");

    return stages;
}
