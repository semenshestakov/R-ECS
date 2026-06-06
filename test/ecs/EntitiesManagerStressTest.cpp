#include <gtest/gtest.h>
#include <random>
#include "ComponentsClass.hpp"
#include "ecs/entities/EntitiesManager.hpp"
#include "ecs/entities/PrefabEntity.hpp"


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

    static PrefabEntity Create2DPrefab(float x = 1.f, float y = 2.f)
    {
        PrefabEntity prefab;
        prefab.AddComponent<Position2d>(x, y);
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


TEST_F(EntitiesManagerStressTest, View_10k)
{
    for (size_t i = 0; i < N; ++i)
        manager.Create(CreatePrefab());

    size_t count = 0;
    auto view = manager.view<Position2d, Position3d>();
    for (auto it = view.begin(); it != view.end(); ++it)
    {
        count++;
    }
    EXPECT_EQ(count, N);
}


TEST_F(EntitiesManagerStressTest, View_10k_ZeroSuitable)
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


TEST_F(EntitiesManagerStressTest, Destroy_10k)
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


TEST_F(EntitiesManagerStressTest, RandomDeletionStress)
{
    constexpr size_t N = 5000;

    std::vector<EntityWrapper> entities;

    for (size_t i = 0; i < N; ++i)
    {
        entities.emplace_back(manager.Create(Create2DPrefab(static_cast<float>(i), 0.f)));
    }

    std::mt19937 rng(42);
    std::shuffle(entities.begin(), entities.end(), rng);

    for (size_t i = 0; i < N / 2; ++i)
    {
        manager.Destroy(entities[i]);
    }

    EXPECT_EQ(manager.size(), N / 2);

    for (size_t i = N / 2; i < N; ++i)
    {
        EXPECT_TRUE(manager.IsAlive(entities[i]));

        const auto& pos = entities[i].GetComponent<Position2d>();
        EXPECT_GE(pos.x, 0.f);
    }
}


TEST_F(EntitiesManagerStressTest, CreateDestroyCreateDestroyStress)
{
    for (size_t i = 0; i < N; ++i)
    {
        auto entity = manager.Create(
            Create2DPrefab(static_cast<float>(i), 0.f));

        EXPECT_TRUE(manager.IsAlive(entity));

        manager.Destroy(entity);

        EXPECT_FALSE(manager.IsAlive(entity));
    }

    EXPECT_EQ(manager.size(), 0);
}


TEST_F(EntitiesManagerStressTest, HugeArchetype_1000Components)
{
    PrefabEntity prefab;

    [&]<size_t... Is>(std::index_sequence<Is...>)
    {
        (prefab.AddComponent<TestComponent<Is>>(), ...);
    }
    (std::make_index_sequence<100>{});

    const auto entity = manager.Create(prefab);

    ValidateEntity(entity,std::make_index_sequence<100>{});
}


TEST_F(EntitiesManagerStressTest, Create1000DifferentArchetypes)
{
    std::vector<EntityWrapper> entities;

    for (size_t archetypeSize = 1; archetypeSize <= 1000; ++archetypeSize)
    {
        PrefabEntity prefab;

        BuildPrefab<1000>(prefab, std::make_index_sequence<1000>{});

        auto entity = manager.Create(prefab);

        entities.push_back(entity);
    }

    EXPECT_EQ(manager.size(), 1000);
}


TEST_F(EntitiesManagerStressTest, ArchetypeIntersectionValidation)
{
    PrefabEntity a;
    PrefabEntity b;
    PrefabEntity c;

    [&]<size_t... Is>(std::index_sequence<Is...>)
    {
    (
        (Is < 500
            ? (void)a.AddComponent<TestComponent<Is>>()
            : (void)0),
        ...
    );

    (
        ((Is >= 250 && Is < 750)
            ? (void)b.AddComponent<TestComponent<Is>>()
            : (void)0),
        ...
    );

    (
        (Is >= 500
            ? (void)c.AddComponent<TestComponent<Is>>()
            : (void)0),
        ...
    );
    }
    (std::make_index_sequence<1000>{});

    const auto ea = manager.Create(a);
    const auto eb = manager.Create(b);
    const auto ec = manager.Create(c);

    ValidateRange<0, 500>(
        ea,
        std::make_index_sequence<1000>{});

    ValidateRange<250, 750>(
        eb,
        std::make_index_sequence<1000>{});

    ValidateRange<500, 1000>(
        ec,
        std::make_index_sequence<1000>{});
}