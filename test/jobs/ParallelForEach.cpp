#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <numeric>
#include <tuple>
#include <vector>

#include "ComponentsClass.hpp"
#include "ecs/entities/EntitiesManager.hpp"
#include "ecs/entities/PrefabEntity.hpp"
#include "ecs/jobs/SerialJobScheduler.hpp"


using namespace ecs;


// IJobScheduler::ParallelForEach is a port-level method, so its behaviour is verified here
// on the always-available SerialJobScheduler (no threading backend required). The TBB
// backend reruns the coverage checks under real parallelism in test/jobs/TbbJobScheduler.cpp.
class ParallelForEachTest : public ::testing::Test
{
protected:
    EntitiesManager manager;
    SerialJobScheduler scheduler;

    // Creates `count` entities, each carrying TestId(index) plus the requested extra components.
    template<typename... Extra>
    void CreateWithId(const std::size_t count, const std::size_t firstId = 0)
    {
        for (std::size_t i = 0; i < count; ++i)
        {
            PrefabEntity prefab;
            prefab.AddComponent<TestId>(static_cast<unsigned int>(firstId + i));
            (prefab.AddComponent<Extra>(), ...);
            manager.Create(prefab);
        }
    }

    template<IsComponent... ComponentCls>
    std::size_t CountVisited(const std::size_t chunksPerTask = 1)
    {
        std::atomic<std::size_t> visited{0};
        scheduler.ParallelForEach(
            manager.chunkView<ComponentCls...>(),
            [&](auto&&...) { visited.fetch_add(1, std::memory_order_relaxed); },
            chunksPerTask);
        return visited.load();
    }
};


TEST_F(ParallelForEachTest, VisitsEveryEntityExactlyOnceAcrossManyChunks)
{
    constexpr std::size_t n = 5000; // > MAX_ENTITIES_IN_CHUNK (1024) -> several chunks
    CreateWithId<Position2d>(n);

    std::vector<std::atomic<int>> seen(n);
    for (auto& s : seen)
        s.store(0);

    scheduler.ParallelForEach(
        manager.chunkView<TestId, Position2d>(),
        [&](TestId& id, Position2d&) {
            seen[id.id].fetch_add(1, std::memory_order_relaxed);
        });

    for (std::size_t i = 0; i < n; ++i)
        EXPECT_EQ(seen[i].load(), 1) << "entity id " << i;
}


TEST_F(ParallelForEachTest, MutatesComponentsInPlace)
{
    constexpr std::size_t n = 3000;
    for (std::size_t i = 0; i < n; ++i)
    {
        PrefabEntity prefab;
        prefab.AddComponent<TestId>(i);
        manager.Create(prefab);
    }

    scheduler.ParallelForEach(
        manager.chunkView<TestId>(),
        [](TestId& testId) { testId.id += 1; });

    std::size_t sum = 0.0;
    for (auto [ids] : manager.view<TestId>())
        sum += ids.id;

    constexpr std::size_t expected = n * (n - 1) / 2.0 + n;
    EXPECT_EQ(sum, expected);
}


TEST_F(ParallelForEachTest, SpansAllArchetypesContainingTheComponents)
{
    constexpr std::size_t onlyPos = 1200;        // archetype {TestId, Position2d}
    constexpr std::size_t posAndHealth = 1500;   // archetype {TestId, Position2d, Health}
    CreateWithId<Position2d>(onlyPos, 0);
    CreateWithId<Position2d, Health>(posAndHealth, onlyPos);

    // Position2d is a subset of both archetypes -> every entity is visited.
    EXPECT_EQ((CountVisited<Position2d>()), onlyPos + posAndHealth);

    // Health narrows to the second archetype only.
    EXPECT_EQ((CountVisited<Position2d, Health>()), posAndHealth);
    EXPECT_EQ((CountVisited<Health>()), posAndHealth);
}


TEST_F(ParallelForEachTest, EmptyViewIsNoOp)
{
    CreateWithId<Position2d>(500);

    // No entity carries Damage -> body never runs.
    EXPECT_EQ((CountVisited<Damage>()), 0u);

    // No entities at all.
    EntitiesManager empty;
    std::atomic<int> calls{0};
    scheduler.ParallelForEach(empty.chunkView<Position2d>(), [&](auto&&...) { ++calls; });
    EXPECT_EQ(calls.load(), 0);

    calls.store(0);

    scheduler.ParallelForEach(manager.chunkView<Position2d>(), [&](auto&&...) { ++calls; });
    EXPECT_EQ(calls.load(), 500);

}


TEST_F(ParallelForEachTest, GrainVariationsAllCoverEveryEntity)
{
    constexpr std::size_t n = 4096; // exactly several full chunks
    CreateWithId<Position2d>(n);

    for (const std::size_t grain : {std::size_t{0}, std::size_t{1}, std::size_t{2},
                                    std::size_t{7}, std::size_t{100000}})
        EXPECT_EQ((CountVisited<Position2d>(grain)), n) << "grain " << grain;
}


TEST_F(ParallelForEachTest, NoComponentsVisitsEveryEntity)
{
    constexpr std::size_t a = 800;
    constexpr std::size_t b = 900;
    CreateWithId<Position2d>(a, 0);
    CreateWithId<Position2d, Health>(b, a);

    // An empty component pack matches every archetype: one visit per alive entity.
    std::atomic<std::size_t> visited{0};
    scheduler.ParallelForEach(
        manager.chunkView<>(),
        [&](auto&&...) { visited.fetch_add(1, std::memory_order_relaxed); });

    EXPECT_EQ(visited.load(), a + b);
}
