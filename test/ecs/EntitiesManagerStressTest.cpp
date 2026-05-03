#include <gtest/gtest.h>
#include "ecs/entities/EntitiesManager.hpp"
#include "ecs/entities/PrefabEntity.hpp"
#include "ComponentsClass.hpp"


using namespace ecs;

constexpr size_t N = 10'000;


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
    PrefabEntity prefab = CreatePrefab();

    for (size_t i = 0; i < N; ++i)
        manager.Create(prefab);

    EXPECT_EQ(manager.size(), N);
}


TEST_F(EntitiesManagerStressTest, Access_100k_Components)
{
    std::vector<Entity> entities;
    entities.reserve(N);

    for (size_t i = 0; i < N; ++i)
        entities.emplace_back(manager.Create(CreatePrefab()).getEntity());

    float sum = 0.f;
    for (auto& e : entities)
    {
        const auto& pos = manager.GetComponent<Position2d>(e);
        sum += pos.x;
    }

    EXPECT_GT(sum, 0.f);
}


TEST_F(EntitiesManagerStressTest, View_100k)
{
    for (size_t i = 0; i < N; ++i)
        manager.Create(CreatePrefab());

    size_t count = 0;
    for (auto [pos2d, pos3d] : manager.view<Position2d, Position3d>())
    {
        count++;
    }
    EXPECT_EQ(count, N);
}


TEST_F(EntitiesManagerStressTest, View_100k_ZeroSuitable)
{
    for (size_t i = 0; i < N; ++i)
        manager.Create(CreatePrefab());

    size_t count = 0;
    for (auto [comp] : manager.view<DestructorTest>())
    {
        count++;
    }
    EXPECT_EQ(count, 0);
}


TEST_F(EntitiesManagerStressTest, Destroy_100k)
{
    std::vector<EntityWrapper> entities;
    entities.reserve(N);

    for (size_t i = 0; i < N; ++i)
        entities.emplace_back(manager.Create(CreatePrefab()));

    for (auto& e : entities)
        manager.Destroy(e);
    EXPECT_EQ(manager.size(), 0);
}


TEST_F(EntitiesManagerStressTest, CreateDestroy_Cycles)
{
    constexpr size_t N = 50'00;
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
