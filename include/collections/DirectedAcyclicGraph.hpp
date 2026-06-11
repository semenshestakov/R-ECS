#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace collections
{

    /**
     * @brief A template-based directed acyclic graph (DAG) for managing dependency relationships.
     *
     * The DirectedAcyclicGraph class provides a robust implementation of a directed acyclic graph
     * specifically designed for dependency management and topological ordering. It allows nodes
     * to be connected through directed edges representing dependencies, and provides automatic
     * cycle detection and topological sorting capabilities.
     *
     * This class is particularly useful in scenarios requiring dependency resolution, such as:
     * - Build systems and task schedulers (determining execution order)
     * - Package managers (resolving installation order)
     * - Data processing pipelines (computing data flow order)
     * - Entity component systems (managing system update order)
     *
     * Key features:
     * - Type-safe graph operations with template-based node types
     * - Automatic cycle detection with detailed error reporting
     * - Topological sorting with parallel execution stages
     * - Move semantics for efficient graph transfers
     * - STL-compatible iterators for graph traversal
     *
     * The graph maintains a collection of nodes, where each node can have multiple dependencies
     * (edges pointing to other nodes). The topological sort algorithm (Kahn's algorithm)
     * produces stages where nodes within the same stage have no dependencies on each other
     * and can be executed in parallel.
     *
     * @tparam NodeType The type of node identifiers used in the graph.
     *                  Must support equality comparison and hash operations.
     *
     * @note The graph ensures acyclic property through runtime checks during topological sorting.
     * @warning Adding a dependency that creates a cycle will only be detected during build()
     *          operation, not during emplace calls.
     *
     * @example
     * @code
     * // Example: Task scheduling system
     * collections::DirectedAcyclicGraph<std::string> taskGraph;
     *
     * // Define task dependencies
     * taskGraph.emplace("compile", "parse");      // compile depends on parse
     * taskGraph.emplace("link", "compile");       // link depends on compile
     * taskGraph.emplace("test", "link");          // test depends on link
     * taskGraph.emplace("package", "test");       // package depends on test
     *
     * @endcode
     */
    template <typename NodeType>
    struct DirectedAcyclicGraph
    {
    private:
        /**
         * @brief Internal node structure storing node data and its dependencies.
         *
         * Each node maintains a set of dependencies (other nodes that must be processed
         * before this node). The dependencies are stored as an unordered_set for O(1)
         * lookup and insertion operations.
         */
        struct Node
        {
        private:
            NodeType data;                              ///< The actual node identifier/value
            std::unordered_set<NodeType> dependencies;  ///< Set of nodes this node depends on

            friend class DirectedAcyclicGraph<NodeType>;

        public:
            /**
             * @brief Default constructor.
             * Creates an empty node with no data and no dependencies.
             */
            Node() = default;

            /**
             * @brief Constructs a node with the given data.
             * @param d The node identifier/value to store.
             */
            Node(const NodeType& d) : data(d) {}

            /**
             * @brief Constructs a node with data and initial dependencies.
             * @param d The node identifier/value to store.
             * @param deps The set of initial dependencies.
             */
            Node(const NodeType& d, std::unordered_set<NodeType> deps)
                : data(d), dependencies(std::move(deps)) {}

            /**
             * @brief Returns an iterator to the beginning of the dependencies set.
             * @return Iterator to the first dependency.
             */
            auto begin() const { return dependencies.begin(); }

            /**
             * @brief Returns an iterator to the end of the dependencies set.
             * @return Iterator to one past the last dependency.
             */
            auto end() const { return dependencies.end(); }
        };

        std::unordered_map<NodeType, Node> m_nodes;  ///< Storage container mapping node identifiers to their node objects

    public:
        using stagesGraph_t = std::vector<std::vector<NodeType>>;

        /**
         * @brief Adds a node to the graph without any dependencies.
         *
         * Creates and adds a new node with the specified identifier. If a node with the same
         * identifier already exists, the operation does nothing (no duplication).
         * This operation is idempotent and safe to call multiple times.
         *
         * @param nodeData The identifier of the node to add.
         *
         * @example
         * @code
         * collections::DirectedAcyclicGraph<int> graph;
         * graph.emplace(1);      // Add node 1
         * graph.emplace(2);      // Add node 2
         * graph.emplace(1);      // No effect - node already exists
         * assert(graph.size() == 2);
         * @endcode
         */
        void emplace(const NodeType& nodeData);

        /**
         * @brief Adds a node with a dependency on another node.
         *
         * Creates or updates a node that depends on the specified dependency node.
         * If the node doesn't exist, it is created. If the dependency node doesn't exist,
         * it is automatically created. The dependency relationship is directional:
         * the node depends on the dependency, meaning the dependency must be processed
         * before this node.
         *
         * This method ensures that both nodes exist in the graph and establishes the
         * dependency relationship. Multiple dependencies can be added to the same node
         * by calling this method multiple times with different dependencies.
         *
         * @param nodeData The node that depends on the dependency.
         * @param dep The node that must be processed before nodeData.
         *
         * @example
         * @code
         * // Create dependency: node 2 depends on node 1
         * graph.emplace(2, 1);    // Creates both nodes and adds dependency
         *
         * // Add multiple dependencies to the same node
         * graph.emplace(4, 2);    // 4 depends on 2
         * graph.emplace(4, 3);    // 4 also depends on 3
         *
         * // Now node 4 requires both nodes 2 and 3 to be processed first
         * @endcode
         */
        void emplace(const NodeType& nodeData, const NodeType& dep);

        /**
         * @brief Performs a topological sort of the graph, producing execution stages.
         *
         * Implements Kahn's algorithm to compute a topological ordering of all nodes in the
         * graph. The algorithm groups nodes into stages where all nodes in the same stage
         * have no dependencies on each other and can be executed in parallel.
         *
         * The algorithm works as follows:
         * 1. Calculate in-degree (number of dependencies) for each node
         * 2. Identify nodes with no dependencies (in-degree = 0)
         * 3. Process nodes in stages, removing them and updating dependent nodes
         * 4. Detect cycles if any nodes remain unprocessed
         *
         * @return A vector of stages, where each stage is a vector of nodes that can be
         *         executed in parallel. Stages are ordered from least to most dependent.
         * @throws std::runtime_error if a cycle is detected in the graph. The error message
         *         includes the number of unprocessed nodes for diagnostic purposes.
         *
         * @complexity O(V + E) where V is the number of nodes and E is the number of dependencies.
         *
         * @example
         * @code
         * // Graph: 1 -> 2 -> 3 (linear chain)
         * graph.emplace(2, 1);
         * graph.emplace(3, 2);
         *
         * auto stages = graph.build();
         * // stages = [[1], [2], [3]]
         *
         * // Graph with parallel nodes: 1 -> 2, 1 -> 3, 2 -> 4, 3 -> 4
         * graph.emplace(2, 1);
         * graph.emplace(3, 1);
         * graph.emplace(4, 2);
         * graph.emplace(4, 3);
         *
         * stages = graph.build();
         * // stages = [[1], [2, 3], [4]]
         * // Nodes 2 and 3 can run in parallel
         * @endcode
         */
        stagesGraph_t build() const;

        /**
         * @brief Returns an iterator to the beginning of the node container.
         *
         * Provides STL-compatible iteration over all nodes in the graph.
         * Each element is a pair of (node_identifier, node_data).
         *
         * @return Iterator to the first node in the unordered map.
         */
        [[nodiscard]] auto begin() const { return m_nodes.begin(); }

        /**
         * @brief Returns an iterator to the end of the node container.
         *
         * @return Iterator to one past the last node in the unordered map.
         */
        [[nodiscard]] auto end() const { return m_nodes.end(); }

        /**
         * @brief Returns the number of nodes currently in the graph.
         *
         * @return The total count of nodes stored in the graph.
         *
         * @example
         * @code
         * graph.emplace(1);
         * graph.emplace(2, 1);
         * assert(graph.size() == 2);
         * @endcode
         */
        [[nodiscard]] size_t size() const { return m_nodes.size(); }

        /**
         * @brief Checks if the graph contains any nodes.
         *
         * @return true if the graph has no nodes, false otherwise.
         */
        [[nodiscard]] bool empty() const { return m_nodes.empty(); }

        /**
         * @brief Checks if a specific node exists in the graph.
         *
         * Performs a lookup to determine if the specified node identifier is present
         * in the graph. This operation is constant time on average.
         *
         * @param node The node identifier to check for existence.
         * @return true if the node exists, false otherwise.
         *
         * @example
         * @code
         * graph.emplace(42);
         * assert(graph.has(42));
         * assert(!graph.has(43));
         * @endcode
         */
        [[nodiscard]] bool has(NodeType node) const { return m_nodes.contains(node); }

        /**
         * @brief Removes all nodes from the graph.
         *
         * Clears the entire graph, destroying all nodes and their dependencies.
         * The graph is left in a valid empty state ready for new operations.
         *
         * @example
         * @code
         * graph.emplace(1);
         * graph.emplace(2, 1);
         * assert(graph.size() == 2);
         *
         * graph.clear();
         * assert(graph.empty());
         * assert(!graph.has(1));
         * @endcode
         */
        void clear() { m_nodes.clear(); }
    };

} // namespace collections

#include "detail/DirectedAcyclicGraph.ipp"