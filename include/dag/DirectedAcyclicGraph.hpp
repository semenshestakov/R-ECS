#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace dag
{

    template <typename NodeType>
    struct DirectedAcyclicGraph
    {
    private:
        struct Node
        {
        private:
            NodeType data;
            std::unordered_set<NodeType> dependencies;

            friend class DirectedAcyclicGraph<NodeType>;
        public:
            Node() = default;
            Node(const NodeType& d) : data(d) {}
            Node(const NodeType& d, std::unordered_set<NodeType> deps) : data(d), dependencies(std::move(deps)) {}

            auto begin() const { return dependencies.begin(); }
            auto end() const { return dependencies.end(); }
        };

        std::unordered_map<NodeType, Node> m_nodes;

    public:
        void emplace(const NodeType& nodeData);
        void emplace(const NodeType& nodeData, const NodeType& dep);
        std::vector<std::vector<NodeType>> build() const;

        [[nodiscard]] auto begin() const { return m_nodes.begin(); }
        [[nodiscard]] auto end() const { return m_nodes.end(); }

        [[nodiscard]] size_t size() const { return m_nodes.size(); }
        [[nodiscard]] bool empty() const { return m_nodes.empty(); }
        [[nodiscard]] bool has(NodeType node) const { return m_nodes.contains(node); }

        void clear() { m_nodes.clear(); }
    };

} // namespace dag
#include "detail/DirectedAcyclicGraph.ipp"