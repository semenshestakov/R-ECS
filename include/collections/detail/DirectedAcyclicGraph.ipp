#pragma once
#include <queue>
#include <stdexcept>
#include "../DirectedAcyclicGraph.hpp"

namespace collections
{

    template<typename NodeType>
    void DirectedAcyclicGraph<NodeType>::emplace(const NodeType& nodeData)
    {
        if (!m_nodes.contains(nodeData))
            m_nodes[nodeData] = Node(nodeData);
    }
    
    template<typename NodeType>
    void DirectedAcyclicGraph<NodeType>::emplace(const NodeType& nodeData, const NodeType& dep)
    {
        // Ensure the node exists
        if (!m_nodes.contains(nodeData))
            m_nodes[nodeData] = Node(nodeData);

        // Ensure the dependency exists
        if (!m_nodes.contains(dep))
            m_nodes[dep] = Node(dep);

        // Add the dependency
        m_nodes[nodeData].dependencies.insert(dep);
    }
  
    template<typename NodeType>
    std::vector<std::vector<NodeType>> DirectedAcyclicGraph<NodeType>::build() const
    {
        std::unordered_map<NodeType, int> inDegree;

        for (auto const& [node, _] : m_nodes)
            inDegree[node] = 0;

        for (auto const& [node, n] : m_nodes)
            for (auto dep : n.dependencies)
                ++inDegree[node];

        std::queue<NodeType> ready;
        for (auto const& [node, deg] : inDegree)
            if (deg == 0)
                ready.push(node);

        std::unordered_set<NodeType> visited;
        std::vector<std::vector<NodeType>> stages;

        while (!ready.empty())
        {
            std::vector<NodeType> stage;
            size_t n = ready.size();

            for (size_t i = 0; i < n; i++)
            {
                auto current = ready.front();
                ready.pop();
                visited.insert(current);
                stage.push_back(current);

                for (auto const& [node, nodeObj] : m_nodes)
                {
                    if (nodeObj.dependencies.count(current))
                    {
                        --inDegree[node];
                        if (inDegree[node] == 0 && visited.count(node) == 0)
                            ready.push(node);
                    }
                }
            }

            stages.push_back(stage);
        }

        if (visited.size() != m_nodes.size())
            throw std::runtime_error("Cycle detected in DAG");

        return stages;
    }


} // namespace dag
