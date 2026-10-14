#include "collections/DirectedAcyclicGraph.hpp"
#include <gtest/gtest.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_set>


using namespace collections;


class DirectedAcyclicGraphTest : public ::testing::Test
{
protected:
    void SetUp() override {
        // Clear any existing data between tests
        graph.clear();
    }

    DirectedAcyclicGraph<int> graph;
};


template<typename NodeType>
void drawGraph(std::vector<std::vector<NodeType>>& stages)
{
    int i = 0 ;
    for (auto stage : stages)
    {
        std::cout << "Stage [" << i << "] [";
        int j = 0;
        for (auto node : stage)
        {
            std::cout << node << ", ";

        }
        std::cout << "]\n";
        i++;
    }
}


// Test basic emplace without dependencies
TEST_F(DirectedAcyclicGraphTest, EmplaceNodeWithoutDependencies)
{
    graph.emplace(1);

    EXPECT_EQ(graph.size(), 1);
    EXPECT_TRUE(graph.has(1));
    EXPECT_FALSE(graph.empty());
}


// Test emplace multiple nodes without dependencies
TEST_F(DirectedAcyclicGraphTest, EmplaceMultipleNodesWithoutDependencies)
{
    graph.emplace(1);
    graph.emplace(2);
    graph.emplace(3);

    EXPECT_EQ(graph.size(), 3);
    EXPECT_TRUE(graph.has(1));
    EXPECT_TRUE(graph.has(2));
    EXPECT_TRUE(graph.has(3));
}


// Test emplace with dependency
TEST_F(DirectedAcyclicGraphTest, EmplaceNodeWithDependency)
{
    graph.emplace(2, 1);

    EXPECT_EQ(graph.size(), 2);
    EXPECT_TRUE(graph.has(1));
    EXPECT_TRUE(graph.has(2));
}


// Test emplace duplicate node
TEST_F(DirectedAcyclicGraphTest, EmplaceDuplicateNode)
{
    graph.emplace(1);
    graph.emplace(1);

    EXPECT_EQ(graph.size(), 1);
    EXPECT_TRUE(graph.has(1));
}


// Test emplace duplicate dependency
TEST_F(DirectedAcyclicGraphTest, EmplaceDuplicateDependency)
{
    graph.emplace(2, 1);
    graph.emplace(2, 1); // Add same dependency again

    EXPECT_EQ(graph.size(), 2);
    EXPECT_TRUE(graph.has(1));
    EXPECT_TRUE(graph.has(2));
}


// Test build with single node
TEST_F(DirectedAcyclicGraphTest, BuildSingleNode)
{
    graph.emplace(1);
    auto stages = graph.build();

    ASSERT_EQ(stages.size(), 1);
    ASSERT_EQ(stages[0].size(), 1);
    EXPECT_EQ(stages[0][0], 1);
}


// Test build with multiple independent nodes
TEST_F(DirectedAcyclicGraphTest, BuildIndependentNodes)
{
    graph.emplace(1);
    graph.emplace(2);
    graph.emplace(3);

    auto stages = graph.build();

    ASSERT_EQ(stages.size(), 1);
    ASSERT_EQ(stages[0].size(), 3);

    // Check that all nodes are in the stage (order may vary)
    std::unordered_set<int> expected = {1, 2, 3};
    std::unordered_set<int> actual(stages[0].begin(), stages[0].end());
    EXPECT_EQ(expected, actual);
}


// Test build with linear dependency chain
TEST_F(DirectedAcyclicGraphTest, BuildLinearDependencyChain)
{
    graph.emplace(3, 2);
    graph.emplace(2, 1);
    graph.emplace(1);

    auto stages = graph.build();

    ASSERT_EQ(stages.size(), 3);
    ASSERT_EQ(stages[0].size(), 1);
    ASSERT_EQ(stages[1].size(), 1);
    ASSERT_EQ(stages[2].size(), 1);

    EXPECT_EQ(stages[0][0], 1);
    EXPECT_EQ(stages[1][0], 2);
    EXPECT_EQ(stages[2][0], 3);
}


// Test build with diamond dependency pattern
TEST_F(DirectedAcyclicGraphTest, BuildDiamondDependency)
{
    // Nodes: 1 -> 2, 1 -> 3, 2 -> 4, 3 -> 4
    graph.emplace(2, 1);
    graph.emplace(3, 1);
    graph.emplace(4, 2);
    graph.emplace(4, 3);
    graph.emplace(1);

    auto stages = graph.build();

    // Stage 0: Node 1
    // Stage 1: Nodes 2 and 3 (parallel)
    // Stage 2: Node 4
    ASSERT_EQ(stages.size(), 3);
    ASSERT_EQ(stages[0].size(), 1);
    ASSERT_EQ(stages[1].size(), 2);
    ASSERT_EQ(stages[2].size(), 1);

    EXPECT_EQ(stages[0][0], 1);

    // Stage 1 should contain both 2 and 3 (order may vary)
    std::unordered_set<int> stage1_set(stages[1].begin(), stages[1].end());
    EXPECT_TRUE(stage1_set.count(2));
    EXPECT_TRUE(stage1_set.count(3));

    EXPECT_EQ(stages[2][0], 4);
}

// Test build with complex dependencies
TEST_F(DirectedAcyclicGraphTest, BuildComplexDependencies)
{
    // Create a more complex DAG
    graph.emplace(2, 1);
    graph.emplace(3, 1);
    graph.emplace(4, 2);
    graph.emplace(5, 3);
    graph.emplace(6, 4);
    graph.emplace(6, 5);
    graph.emplace(1);

    auto stages = graph.build();

    // Verify all nodes are processed
    size_t total_nodes = 0;
    for (const auto& stage : stages) {
        total_nodes += stage.size();
    }
    EXPECT_EQ(total_nodes, 6);

    // Verify topological order constraints
    std::unordered_map<int, int> node_to_stage;
    for (size_t i = 0; i < stages.size(); ++i) {
        for (int node : stages[i]) {
            node_to_stage[node] = i;
        }
    }

    // Check dependencies appear in earlier stages
    EXPECT_LT(node_to_stage[1], node_to_stage[2]);
    EXPECT_LT(node_to_stage[1], node_to_stage[3]);
    EXPECT_LT(node_to_stage[2], node_to_stage[4]);
    EXPECT_LT(node_to_stage[3], node_to_stage[5]);
    EXPECT_LT(node_to_stage[4], node_to_stage[6]);
    EXPECT_LT(node_to_stage[5], node_to_stage[6]);
}


// Test build with cycle detection
TEST_F(DirectedAcyclicGraphTest, BuildWithCycleThrowsException)
{
    // Create a cycle: 1 -> 2 -> 1
    graph.emplace(2, 1);
    graph.emplace(1, 2);

    EXPECT_THROW(graph.build(), std::runtime_error);
}


// Test build with self-loop cycle
TEST_F(DirectedAcyclicGraphTest, BuildWithSelfLoopThrowsException)
{
    // Self-loop: 1 depends on 1
    graph.emplace(1, 1);
    EXPECT_THROW(graph.build(), std::runtime_error);
}


// Test build with missing dependency (should throw)
TEST_F(DirectedAcyclicGraphTest, BuildWithMissingDependency)
{
    // Try to add dependency to non-existent node through emplace with dependency
    // This should work because emplace creates the dependency node
    graph.emplace(2, 1); // Creates both nodes
    graph.emplace(3, 4); // Creates both nodes

    // This should not throw because all dependencies exist
    EXPECT_NO_THROW(graph.build());
}


// Test build with string nodes
TEST(DirectedAcyclicGraphTestString, BuildWithStringNodes)
{
    DirectedAcyclicGraph<std::string> string_graph;

    string_graph.emplace("task_b", "task_a");
    string_graph.emplace("task_c", "task_a");
    string_graph.emplace("task_d", "task_b");
    string_graph.emplace("task_d", "task_c");
    string_graph.emplace("task_a");

    auto stages = string_graph.build();

    // Verify topological order
    EXPECT_EQ(stages[0][0], "task_a");

    std::unordered_set<std::string> stage1_set(stages[1].begin(), stages[1].end());
    EXPECT_TRUE(stage1_set.count("task_b"));
    EXPECT_TRUE(stage1_set.count("task_c"));

    EXPECT_EQ(stages[2][0], "task_d");
}


// Test clear functionality
TEST_F(DirectedAcyclicGraphTest, ClearGraph)
{
    graph.emplace(1);
    graph.emplace(2);
    graph.emplace(3, 1);

    EXPECT_EQ(graph.size(), 3);
    graph.clear();
    EXPECT_TRUE(graph.empty());
    EXPECT_EQ(graph.size(), 0);
}


// Test has method
TEST_F(DirectedAcyclicGraphTest, HasNode)
{
    EXPECT_FALSE(graph.has(1));

    graph.emplace(1);
    EXPECT_TRUE(graph.has(1));

    graph.emplace(2, 1);
    EXPECT_TRUE(graph.has(2));
}


// Test build with large number of nodes
TEST_F(DirectedAcyclicGraphTest, BuildLargeGraph)
{
    const int num_nodes = 100;

    // Create a linear chain: 1 -> 2 -> 3 -> ... -> 100
    for (int i = 2; i <= num_nodes; ++i) {
        graph.emplace(i, i - 1);
    }
    graph.emplace(1);

    auto stages = graph.build();

    EXPECT_EQ(stages.size(), num_nodes);
    for (int i = 0; i < num_nodes; ++i) {
        ASSERT_EQ(stages[i].size(), 1);
        EXPECT_EQ(stages[i][0], i + 1);
    }
}


// Test iteration over graph nodes
TEST_F(DirectedAcyclicGraphTest, IterateOverNodes)
{
    graph.emplace(1);
    graph.emplace(2);
    graph.emplace(3);

    std::unordered_set<int> expected = {1, 2, 3};
    std::unordered_set<int> actual;

    for (const auto& [node, _] : graph) {
        actual.insert(node);
    }

    EXPECT_EQ(expected, actual);
}


// Test build with nodes that have multiple dependencies
TEST_F(DirectedAcyclicGraphTest, BuildWithMultipleDependencies)
{
    graph.emplace(4, 1);
    graph.emplace(4, 2);
    graph.emplace(4, 3);
    graph.emplace(1);
    graph.emplace(2);
    graph.emplace(3);

    auto stages = graph.build();

    // Stage 0: 1,2,3 (order may vary)
    // Stage 1: 4
    ASSERT_EQ(stages.size(), 2);
    ASSERT_EQ(stages[0].size(), 3);
    ASSERT_EQ(stages[1].size(), 1);

    std::unordered_set<int> stage0_set(stages[0].begin(), stages[0].end());
    EXPECT_TRUE(stage0_set.count(1));
    EXPECT_TRUE(stage0_set.count(2));
    EXPECT_TRUE(stage0_set.count(3));

    EXPECT_EQ(stages[1][0], 4);
}


// Test that build returns stages in correct order for complex dependencies
TEST_F(DirectedAcyclicGraphTest, BuildComplexOrder)
{
    graph.emplace(3, 1);
    graph.emplace(4, 2);
    graph.emplace(5, 3);
    graph.emplace(5, 4);
    graph.emplace(1);
    graph.emplace(2);

    auto stages = graph.build();

    // Stage 0 should contain 1 and 2 (no dependencies)
    // Stage 1 should contain 3 and 4 (depend on stage 0)
    // Stage 2 should contain 5 (depends on stage 1)

    ASSERT_EQ(stages.size(), 3);
    ASSERT_EQ(stages[0].size(), 2);
    ASSERT_EQ(stages[1].size(), 2);
    ASSERT_EQ(stages[2].size(), 1);

    std::unordered_set<int> stage0_set(stages[0].begin(), stages[0].end());
    EXPECT_TRUE(stage0_set.count(1));
    EXPECT_TRUE(stage0_set.count(2));

    std::unordered_set<int> stage1_set(stages[1].begin(), stages[1].end());
    EXPECT_TRUE(stage1_set.count(3));
    EXPECT_TRUE(stage1_set.count(4));

    EXPECT_EQ(stages[2][0], 5);
}


TEST_F(DirectedAcyclicGraphTest, BuildComplexGraph_WithDependencyValidation)
{
    graph.emplace(3, 1);
    graph.emplace(2, 1);
    graph.emplace(4, 2);
    graph.emplace(4, 3);

    graph.emplace(5, 2);
    graph.emplace(6, 3);

    graph.emplace(7, 4);
    graph.emplace(7, 5);
    graph.emplace(7, 6);

    auto stages = graph.build();

    ASSERT_EQ(stages.size(), 4);

    {
        std::unordered_set<int> s0(stages[0].begin(), stages[0].end());
        EXPECT_EQ(s0.size(), 1);
        EXPECT_TRUE(s0.contains(1));
    }

    {
        std::unordered_set<int> s1(stages[1].begin(), stages[1].end());
        EXPECT_EQ(s1.size(), 2);
        EXPECT_TRUE(s1.contains(2));
        EXPECT_TRUE(s1.contains(3));
    }

    {
        std::unordered_set<int> s2(stages[2].begin(), stages[2].end());
        EXPECT_EQ(s2.size(), 3);
        EXPECT_TRUE(s2.contains(4));
        EXPECT_TRUE(s2.contains(5));
        EXPECT_TRUE(s2.contains(6));
    }

    {
        std::unordered_set<int> s3(stages[3].begin(), stages[3].end());
        EXPECT_EQ(s3.size(), 1);
        EXPECT_TRUE(s3.contains(7));
    }


    std::unordered_map<int, int> stageIndex;

    for (int i = 0; i < stages.size(); i++)
    {
        for (auto node : stages[i])
        {
            stageIndex[node] = i;
        }
    }

    EXPECT_LT(stageIndex[1], stageIndex[2]);
    EXPECT_LT(stageIndex[1], stageIndex[3]);

    EXPECT_LT(stageIndex[2], stageIndex[4]);
    EXPECT_LT(stageIndex[3], stageIndex[4]);

    EXPECT_LT(stageIndex[2], stageIndex[5]);

    EXPECT_LT(stageIndex[3], stageIndex[6]);

    EXPECT_LT(stageIndex[4], stageIndex[7]);
    EXPECT_LT(stageIndex[5], stageIndex[7]);
    EXPECT_LT(stageIndex[6], stageIndex[7]);
}

TEST_F(DirectedAcyclicGraphTest, BuildVeryComplexGraph)
{
///        1            Stage 0
///     /  |  \
///    2   3   4        Stage 1
///   / \ / \ / \
///  5   6   7   8      Stage 2
///   \ / \ / \ /
///    9   10 11        Stage 3
///    \   |  /
///       12            Stage 4
///      /  \
///    13    14         Stage 5
///      \  /
///       15            Stage 6

    graph.emplace(2, 1);
    graph.emplace(3, 1);
    graph.emplace(4, 1);

    graph.emplace(5, 2);
    graph.emplace(6, 2);
    graph.emplace(6, 3);
    graph.emplace(7, 3);
    graph.emplace(7, 4);
    graph.emplace(8, 4);

    graph.emplace(9, 5);
    graph.emplace(9, 6);

    graph.emplace(10, 6);
    graph.emplace(10, 7);

    graph.emplace(11, 7);
    graph.emplace(11, 8);

    graph.emplace(12, 9);
    graph.emplace(12, 10);
    graph.emplace(12, 11);

    graph.emplace(13, 12);
    graph.emplace(14, 12);

    graph.emplace(15, 13);
    graph.emplace(15, 14);

    auto stages = graph.build();

    ASSERT_EQ(stages.size(), 7);

    // --- Stage 0 ---
    {
        std::unordered_set<int> s(stages[0].begin(), stages[0].end());
        EXPECT_TRUE(s.contains(1));
    }

    // --- Stage 1 ---
    {
        std::unordered_set<int> s(stages[1].begin(), stages[1].end());
        EXPECT_TRUE(s.contains(2));
        EXPECT_TRUE(s.contains(3));
        EXPECT_TRUE(s.contains(4));
    }

    // --- Stage 2 ---
    {
        std::unordered_set<int> s(stages[2].begin(), stages[2].end());
        EXPECT_TRUE(s.contains(5));
        EXPECT_TRUE(s.contains(6));
        EXPECT_TRUE(s.contains(7));
        EXPECT_TRUE(s.contains(8));
    }

    // --- Stage 3 ---
    {
        std::unordered_set<int> s(stages[3].begin(), stages[3].end());
        EXPECT_TRUE(s.contains(9));
        EXPECT_TRUE(s.contains(10));
        EXPECT_TRUE(s.contains(11));
    }

    // --- Stage 4 ---
    {
        std::unordered_set<int> s(stages[4].begin(), stages[4].end());
        EXPECT_TRUE(s.contains(12));
    }

    // --- Stage 5 ---
    {
        std::unordered_set<int> s(stages[5].begin(), stages[5].end());
        EXPECT_TRUE(s.contains(13));
        EXPECT_TRUE(s.contains(14));
    }

    // --- Stage 6 ---
    {
        std::unordered_set<int> s(stages[6].begin(), stages[6].end());
        EXPECT_TRUE(s.contains(15));
    }

    std::unordered_map<int, int> stageIndex;

    for (int i = 0; i < stages.size(); i++)
        for (auto node : stages[i])
            stageIndex[node] = i;


    EXPECT_LT(stageIndex[1], stageIndex[2]);
    EXPECT_LT(stageIndex[1], stageIndex[3]);
    EXPECT_LT(stageIndex[1], stageIndex[4]);

    EXPECT_LT(stageIndex[2], stageIndex[5]);
    EXPECT_LT(stageIndex[2], stageIndex[6]);
    EXPECT_LT(stageIndex[3], stageIndex[6]);

    EXPECT_LT(stageIndex[3], stageIndex[7]);
    EXPECT_LT(stageIndex[4], stageIndex[7]);

    EXPECT_LT(stageIndex[4], stageIndex[8]);

    EXPECT_LT(stageIndex[5], stageIndex[9]);
    EXPECT_LT(stageIndex[6], stageIndex[9]);

    EXPECT_LT(stageIndex[6], stageIndex[10]);
    EXPECT_LT(stageIndex[7], stageIndex[10]);

    EXPECT_LT(stageIndex[7], stageIndex[11]);
    EXPECT_LT(stageIndex[8], stageIndex[11]);

    EXPECT_LT(stageIndex[9], stageIndex[12]);
    EXPECT_LT(stageIndex[10], stageIndex[12]);
    EXPECT_LT(stageIndex[11], stageIndex[12]);

    EXPECT_LT(stageIndex[12], stageIndex[13]);
    EXPECT_LT(stageIndex[12], stageIndex[14]);

    EXPECT_LT(stageIndex[13], stageIndex[15]);
    EXPECT_LT(stageIndex[14], stageIndex[15]);
}


TEST_F(DirectedAcyclicGraphTest, BuildLongChainWithLocalDiamonds)
{
/// 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 → 10
///
///      2
///     / \
///    11 12
///     \ /
///      3
///
///           5
///          / \
///         13 14
///          \ /
///           6
///
///                8
///               / \
///              15 16
///               \ /
///                9

    for (int i = 2; i <= 10; i++)
    {
        graph.emplace(i, i - 1);
    }


    graph.emplace(11, 2);
    graph.emplace(12, 2);
    graph.emplace(3, 11);
    graph.emplace(3, 12);


    graph.emplace(13, 5);
    graph.emplace(14, 5);
    graph.emplace(6, 13);
    graph.emplace(6, 14);

    graph.emplace(15, 8);
    graph.emplace(16, 8);
    graph.emplace(9, 15);
    graph.emplace(9, 16);

    auto stages = graph.build();

    ASSERT_GE(stages.size(), 10);

    std::unordered_map<int, int> stageIndex;

    for (int i = 0; i < stages.size(); i++)
        for (auto node : stages[i])
            stageIndex[node] = i;

    for (int i = 2; i <= 10; i++)
    {
        EXPECT_LT(stageIndex[i - 1], stageIndex[i]);
    }

    EXPECT_LT(stageIndex[2], stageIndex[11]);
    EXPECT_LT(stageIndex[2], stageIndex[12]);
    EXPECT_LT(stageIndex[11], stageIndex[3]);
    EXPECT_LT(stageIndex[12], stageIndex[3]);

    EXPECT_LT(stageIndex[5], stageIndex[13]);
    EXPECT_LT(stageIndex[5], stageIndex[14]);
    EXPECT_LT(stageIndex[13], stageIndex[6]);
    EXPECT_LT(stageIndex[14], stageIndex[6]);

    EXPECT_LT(stageIndex[8], stageIndex[15]);
    EXPECT_LT(stageIndex[8], stageIndex[16]);
    EXPECT_LT(stageIndex[15], stageIndex[9]);
    EXPECT_LT(stageIndex[16], stageIndex[9]);
}


TEST_F(DirectedAcyclicGraphTest, DiamondInsideOneBranch_AndLongOtherBranch_NoMerge)
{
//       1
//      / \
//     2   3
//    / \   \
//   8   9   4
//    \ /    \
//    10     5
//     |     |
//    11     6
//           |
//           7

    graph.emplace(2, 1);
    graph.emplace(3, 1);

    graph.emplace(8, 2);
    graph.emplace(9, 2);
    graph.emplace(10, 8);
    graph.emplace(10, 9);
    graph.emplace(11, 10);

    graph.emplace(4, 3);
    graph.emplace(5, 4);
    graph.emplace(6, 5);
    graph.emplace(7, 6);

    auto stages = graph.build();

    std::unordered_map<int, int> stageIndex;
    for (int i = 0; i < stages.size(); i++) {
        for (auto node : stages[i]) {
            stageIndex[node] = i;
        }
    }

    EXPECT_LT(stageIndex[1], stageIndex[2]);
    EXPECT_LT(stageIndex[1], stageIndex[3]);

    EXPECT_LT(stageIndex[2], stageIndex[8]);
    EXPECT_LT(stageIndex[2], stageIndex[9]);
    EXPECT_LT(stageIndex[8], stageIndex[10]);
    EXPECT_LT(stageIndex[9], stageIndex[10]);
    EXPECT_LT(stageIndex[10], stageIndex[11]);

    EXPECT_LT(stageIndex[3], stageIndex[4]);
    EXPECT_LT(stageIndex[4], stageIndex[5]);
    EXPECT_LT(stageIndex[5], stageIndex[6]);
    EXPECT_LT(stageIndex[6], stageIndex[7]);

    EXPECT_EQ(stageIndex[2], 1);
    EXPECT_EQ(stageIndex[3], 1);

    EXPECT_EQ(stageIndex[8], 2);
    EXPECT_EQ(stageIndex[9], 2);
    EXPECT_EQ(stageIndex[4], 2);

    EXPECT_EQ(stageIndex[10], 3);
    EXPECT_EQ(stageIndex[5], 3);

    EXPECT_EQ(stageIndex[11], 4);
    EXPECT_EQ(stageIndex[6], 4);

    EXPECT_EQ(stageIndex[7], 5);

    EXPECT_LE(stageIndex[10], stageIndex[5]);
    EXPECT_LE(stageIndex[10], stageIndex[6]);
    EXPECT_LE(stageIndex[10], stageIndex[7]);

    EXPECT_LE(stageIndex[11], stageIndex[6]);
    EXPECT_LT(stageIndex[11], stageIndex[7]);

    EXPECT_LE(stageIndex[5], stageIndex[10]);
    EXPECT_LE(stageIndex[5], stageIndex[11]);

    EXPECT_EQ(stageIndex[6], stageIndex[11]);

    EXPECT_EQ(stages.size(), 6);

    int total_nodes = 0;
    for (const auto& stage : stages)
    {
        total_nodes += stage.size();
    }
    EXPECT_EQ(total_nodes, 11);
}

// Test empty graph build
TEST_F(DirectedAcyclicGraphTest, BuildEmptyGraph)
{
    auto stages = graph.build();
    EXPECT_TRUE(stages.empty());
}


// Test that dependencies are properly maintained after emplace
TEST_F(DirectedAcyclicGraphTest, DependenciesMaintained)
{
    graph.emplace(2, 1);
    graph.emplace(3, 2);

    // Check that we can access dependencies through iteration
    int count = 0;
    for (const auto& [node, node_data] : graph)
    {
        if (node == 2)
        {
            for (auto dep : node_data)
            {
                EXPECT_EQ(dep, 1);
                count++;
            }
        }
        if (node == 3)
        {
            for (auto dep : node_data)
            {
                EXPECT_EQ(dep, 2);
                count++;
            }
        }
    }
    EXPECT_EQ(count, 2);
}


enum EdgeKind : std::uint8_t
{
    NONE   = 0b000,
    DIRECT = 0b010,
    DATA   = 0b100,
};

using FlagGraph = DirectedAcyclicGraph<int, std::uint8_t>;


TEST_F(DirectedAcyclicGraphTest, EdgeReturnsNullWhenNoEdgeExists)
{
    graph.emplace(1);
    graph.emplace(2);

    EXPECT_EQ(graph.edge(2, 1), nullptr);
}


TEST_F(DirectedAcyclicGraphTest, EdgeReturnsNullWhenSourceMissing)
{
    graph.emplace(2, 1);

    EXPECT_EQ(graph.edge(42, 1), nullptr);
}


TEST_F(DirectedAcyclicGraphTest, EdgeExistsWithDefaultPayload)
{
    graph.emplace(2, 1);

    EXPECT_NE(graph.edge(2, 1), nullptr);
    EXPECT_EQ(graph.edge(1, 2), nullptr);
}


TEST(DirectedAcyclicGraphEdgeData, EmplaceWithPayloadStoresIt)
{
    FlagGraph graph;
    graph.emplace(2, 1, EdgeKind::DIRECT);

    ASSERT_NE(graph.edge(2, 1), nullptr);
    EXPECT_EQ(*graph.edge(2, 1), EdgeKind::DIRECT);
}


TEST(DirectedAcyclicGraphEdgeData, EmplaceWithoutPayloadDoesNotClobberExisting)
{
    FlagGraph graph;
    graph.emplace(2, 1, EdgeKind::DATA);
    graph.emplace(2, 1);

    ASSERT_NE(graph.edge(2, 1), nullptr);
    EXPECT_EQ(*graph.edge(2, 1), EdgeKind::DATA);
}


TEST(DirectedAcyclicGraphEdgeData, EmplaceWithPayloadOverwritesExisting)
{
    FlagGraph graph;
    graph.emplace(2, 1, EdgeKind::DIRECT);
    graph.emplace(2, 1, EdgeKind::DATA);

    ASSERT_NE(graph.edge(2, 1), nullptr);
    EXPECT_EQ(*graph.edge(2, 1), EdgeKind::DATA);
}


TEST(DirectedAcyclicGraphEdgeData, DistinctEdgesKeepDistinctPayloads)
{
    FlagGraph graph;
    graph.emplace(3, 1, EdgeKind::DIRECT);
    graph.emplace(3, 2, EdgeKind::DATA);

    ASSERT_NE(graph.edge(3, 1), nullptr);
    ASSERT_NE(graph.edge(3, 2), nullptr);
    EXPECT_EQ(*graph.edge(3, 1), EdgeKind::DIRECT);
    EXPECT_EQ(*graph.edge(3, 2), EdgeKind::DATA);
}


TEST(DirectedAcyclicGraphEdgeData, EdgesAccessorExposesPayloads)
{
    FlagGraph graph;
    graph.emplace(3, 1, EdgeKind::DIRECT);
    graph.emplace(3, 2, EdgeKind::DATA);

    int seen = 0;
    for (const auto& [node, nodeData] : graph)
    {
        if (node != 3)
            continue;

        for (const auto& [dep, kind] : nodeData.edges())
        {
            if (dep == 1)
                EXPECT_EQ(kind, EdgeKind::DIRECT);
            if (dep == 2)
                EXPECT_EQ(kind, EdgeKind::DATA);
            ++seen;
        }
    }
    EXPECT_EQ(seen, 2);
}


TEST(DirectedAcyclicGraphEdgeData, KeyIterationStillYieldsDependencyNodes)
{
    FlagGraph graph;
    graph.emplace(3, 1, EdgeKind::DIRECT);
    graph.emplace(3, 2, EdgeKind::DATA);

    std::unordered_set<int> deps;
    for (const auto& [node, nodeData] : graph)
    {
        if (node != 3)
            continue;

        for (auto dep : nodeData)
            deps.insert(dep);
    }

    EXPECT_EQ(deps, (std::unordered_set<int>{1, 2}));
}


TEST(DirectedAcyclicGraphEdgeData, BuildIgnoresEdgePayload)
{
    FlagGraph graph;
    graph.emplace(2, 1, EdgeKind::DIRECT);
    graph.emplace(3, 2, EdgeKind::DATA);

    auto stages = graph.build();

    ASSERT_EQ(stages.size(), 3);
    ASSERT_EQ(stages[0].size(), 1);
    EXPECT_EQ(stages[0][0], 1);
    EXPECT_EQ(stages[1][0], 2);
    EXPECT_EQ(stages[2][0], 3);
}


TEST(DirectedAcyclicGraphEdgeData, ClearRemovesEdges)
{
    FlagGraph graph;
    graph.emplace(2, 1, EdgeKind::DIRECT);

    graph.clear();

    EXPECT_TRUE(graph.empty());
    EXPECT_EQ(graph.edge(2, 1), nullptr);
}


struct AccessMask
{
    bool read = false;
    bool write = false;

    bool operator==(const AccessMask& other) const
    {
        return read == other.read && write == other.write;
    }
};

TEST(DirectedAcyclicGraphEdgeData, SupportsStructPayload)
{
    DirectedAcyclicGraph<int, AccessMask> graph;
    graph.emplace(2, 1, AccessMask{true, false});

    ASSERT_NE(graph.edge(2, 1), nullptr);
    EXPECT_EQ(*graph.edge(2, 1), (AccessMask{true, false}));
    EXPECT_EQ(graph.build().size(), 2);
}