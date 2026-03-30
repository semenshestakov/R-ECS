#include <gtest/gtest.h>
#include "ecs/entities/EntitiesManager.hpp"
#include "ecs/entities/PrefabEntity.hpp"
#include "ComponentsClass.hpp"


using namespace ecs;


class EntitiesManagerStressTest : public ::testing::Test
{
protected:
    EntitiesManager manager;

    static PrefabEntity CreatePrefab()
    {
        PrefabEntity prefab;
        prefab.AddComponent<Position2d>(1.f, 2.f);
        prefab.AddComponent<Position3d>();
        return prefab;
    }
};


TEST_F(EntitiesManagerStressTest, Create_100k_Entities)
{
    constexpr size_t N = 100'000;
    const auto start = std::chrono::high_resolution_clock::now();

    std::vector<EntityWrapper> entities;
    entities.reserve(N);

    for (size_t i = 0; i < N; ++i)
    {
        entities.emplace_back(manager.Create(CreatePrefab()));
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_EQ(manager.size(), N);

    std::cout << "[Create 100k] Time: " << duration.count() << " ms\n";
}


TEST_F(EntitiesManagerStressTest, Access_100k_Components)
{
    constexpr size_t N = 100'000;

    std::vector<Entity> entities;
    entities.reserve(N);

    for (size_t i = 0; i < N; ++i)
        entities.emplace_back(manager.Create(CreatePrefab()).getEntity());

    const auto start = std::chrono::high_resolution_clock::now();

    float sum = 0.f;

    for (auto& e : entities)
    {
        const auto& pos = manager.GetComponent<Position2d>(e);
        sum += pos.x;
    }

    const auto end = std::chrono::high_resolution_clock::now();

    EXPECT_GT(sum, 0.f);

    std::cout << "[Access 100k] Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n";
}


TEST_F(EntitiesManagerStressTest, View_100k)
{
    constexpr size_t N = 100'000;

    for (size_t i = 0; i < N; ++i)
        manager.Create(CreatePrefab());

    const auto start = std::chrono::high_resolution_clock::now();

    size_t count = 0;

    for (auto [pos2d, pos3d] : manager.view<Position2d, Position3d>())
    {
        count++;
    }

    const auto end = std::chrono::high_resolution_clock::now();

    EXPECT_EQ(count, N);

    std::cout << "[View 100k] Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n";
}


TEST_F(EntitiesManagerStressTest, View_100k_ZeroSuitable)
{
    constexpr size_t N = 100'000;

    for (size_t i = 0; i < N; ++i)
        manager.Create(CreatePrefab());

    const auto start = std::chrono::high_resolution_clock::now();

    size_t count = 0;

    for (auto [comp] : manager.view<DestructorTest>())
    {
        count++;
    }

    const auto end = std::chrono::high_resolution_clock::now();

    EXPECT_EQ(count, 0);

    std::cout << "[View 100k ZeroSuitable] Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n";
}


TEST_F(EntitiesManagerStressTest, Destroy_100k)
{
    constexpr size_t N = 100'000;

    std::vector<EntityWrapper> entities;
    entities.reserve(N);

    for (size_t i = 0; i < N; ++i)
        entities.emplace_back(manager.Create(CreatePrefab()));

    const auto start = std::chrono::high_resolution_clock::now();

    for (auto& e : entities)
        manager.Destroy(e);

    const auto end = std::chrono::high_resolution_clock::now();

    EXPECT_EQ(manager.size(), 0);

    std::cout << "[Destroy 100k] Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n";
}


TEST_F(EntitiesManagerStressTest, CreateDestroy_Cycles)
{
    constexpr size_t N = 50'000;
    constexpr size_t cycles = 5;

    for (size_t c = 0; c < cycles; ++c)
    {
        std::vector<EntityWrapper> entities;
        entities.reserve(N);

        for (size_t i = 0; i < N; ++i)
            entities.emplace_back(manager.Create(CreatePrefab()));

        for (auto& e : entities)
            manager.Destroy(e);

        EXPECT_EQ(manager.size(), 0);
    }
}
