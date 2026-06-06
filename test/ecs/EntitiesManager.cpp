#include <gtest/gtest.h>
#include "ecs/entities/EntitiesManager.hpp"

#include <unordered_set>

#include "ComponentsClass.hpp"
#include "ecs/entities/PrefabEntity.hpp"


using namespace ecs;

class EntitiesManagerTest : public ::testing::Test
{
protected:
    EntitiesManager manager;

    static PrefabEntity Create2DPrefab(float x = 1.f, float y = 2.f)
    {
        PrefabEntity prefab;
        prefab.AddComponent<Position2d>(x, y);
        return prefab;
    }

    static PrefabEntity Create3DPrefab()
    {
        PrefabEntity prefab;
        prefab.AddComponent<Position3d>();
        return prefab;
    }

    static PrefabEntity CreateMixedPrefab()
    {
        PrefabEntity prefab;
        prefab.AddComponent<Position2d>(10.f, 20.f);
        prefab.AddComponent<Position3d>();
        return prefab;
    }

    static PrefabEntity CreateTestIdPrefab()
    {
        PrefabEntity prefab;
        prefab.AddComponent<TestId>();
        return prefab;
    }
};


TEST_F(EntitiesManagerTest, CreateEntity_FromPrefab)
{
    auto prefab = Create2DPrefab(5.f, 6.f);

    const EntityWrapper entity = manager.Create(prefab);

    EXPECT_TRUE(manager.IsAlive(entity));
    EXPECT_EQ(manager.size(), 1);
}


TEST_F(EntitiesManagerTest, GetComponent_ReturnsCorrectDataUseManger)
{
    auto prefab = Create2DPrefab(3.f, 4.f);
    const auto entity = manager.Create(prefab);

    const auto& pos = manager.GetComponent<Position2d>(entity.getEntity());

    EXPECT_FLOAT_EQ(pos.x, 3.f);
    EXPECT_FLOAT_EQ(pos.y, 4.f);
}


TEST_F(EntitiesManagerTest, GetComponent_ReturnsCorrectDataEntityWrapper)
{
    auto prefab = Create2DPrefab(3.f, 4.f);
    const auto entity = manager.Create(prefab);

    const auto& pos = entity.GetComponent<Position2d>();
    EXPECT_FLOAT_EQ(pos.x, 3.f);
    EXPECT_FLOAT_EQ(pos.y, 4.f);
}


TEST_F(EntitiesManagerTest, TryGetComponent_ReturnsNullIfNotExists)
{
    auto prefab = Create2DPrefab();
    auto entity = manager.Create(prefab);

    auto* pos3d = entity.TryGetComponent<Position3d>();

    EXPECT_EQ(pos3d, nullptr);
}


TEST_F(EntitiesManagerTest, MultipleComponents_WorkCorrectly)
{
    auto prefab = CreateMixedPrefab();
    auto entity = manager.Create(prefab);

    const auto& pos2d = entity.GetComponent<Position2d>();
    const auto& pos3d = entity.GetComponent<Position3d>();

    EXPECT_FLOAT_EQ(pos2d.x, 10.f);
    EXPECT_FLOAT_EQ(pos2d.y, 20.f);

    EXPECT_FLOAT_EQ(pos3d.x, 1.f);
    EXPECT_FLOAT_EQ(pos3d.y, 2.f);
    EXPECT_FLOAT_EQ(pos3d.z, 3.f);
}


TEST_F(EntitiesManagerTest, DestroyEntity_RemovesEntity)
{
    auto prefab = Create2DPrefab();
    const auto entity = manager.Create(prefab);

    manager.Destroy(entity);

    EXPECT_FALSE(manager.IsAlive(entity));
    EXPECT_EQ(manager.size(), 0);
}


TEST_F(EntitiesManagerTest, DestroyAndCreate_ReusesEntity)
{
    auto prefab1 = Create2DPrefab();
    const auto e1 = manager.Create(prefab1);
    manager.Destroy(e1);

    auto prefab2 = Create2DPrefab();
    const auto e2 = manager.Create(prefab2);

    EXPECT_TRUE(manager.IsAlive(e2));
    EXPECT_EQ(manager.size(), 1);
}


TEST_F(EntitiesManagerTest, Destructor_IsCalled)
{
    DestructorTest::testValue = false;

    PrefabEntity prefab;
    prefab.AddComponent<DestructorTest>();

    const auto entity = manager.Create(prefab);

    manager.Destroy(entity);

    EXPECT_TRUE(DestructorTest::testValue);
}


TEST_F(EntitiesManagerTest, View_SingleComponent)
{
    manager.Create(Create2DPrefab());
    manager.Create(Create2DPrefab());

    size_t count = 0;

    for (auto [pos] : manager.view<Position2d>())
    {
        EXPECT_FLOAT_EQ(pos.x, 1.f);
        count++;
    }

    EXPECT_EQ(count, 2);
}


TEST_F(EntitiesManagerTest, View_MultipleComponents)
{
    manager.Create(CreateMixedPrefab());
    manager.Create(CreateMixedPrefab());

    size_t count = 0;

    for (auto [pos2d, pos3d] : manager.view<Position2d, Position3d>())
    {
        EXPECT_FLOAT_EQ(pos2d.x, 10.f);
        EXPECT_FLOAT_EQ(pos3d.z, 3.f);
        count++;
    }

    EXPECT_EQ(count, 2);
}

TEST_F(EntitiesManagerTest, View_FiltersEntities)
{
    manager.Create(Create2DPrefab());
    manager.Create(Create3DPrefab());

    size_t count = 0;

    for (auto [pos2d] : manager.view<Position2d>())
    {
        count++;
    }

    EXPECT_EQ(count, 1);
}


TEST_F(EntitiesManagerTest, ReuseEntityIds)
{
    const auto entity1 = manager.Create(Create2DPrefab());
    manager.Destroy(entity1);

    const auto entity2 = manager.Create(Create2DPrefab());
    manager.Create(Create2DPrefab());
    manager.Destroy(entity2);

    EXPECT_EQ(entity1.getEntity().id, entity2.getEntity().id);
    EXPECT_NE(entity1.getEntity().version, entity2.getEntity().version);
    EXPECT_EQ(entity1.getEntity().version + 1, entity2.getEntity().version);

    const auto entity3 = manager.Create(Create2DPrefab());
    manager.Destroy(entity3);

    EXPECT_EQ(entity1.getEntity().id, entity3.getEntity().id);
    EXPECT_NE(entity2.getEntity().version, entity3.getEntity().version);
    EXPECT_EQ(entity2.getEntity().version + 1, entity3.getEntity().version);
}


TEST_F(EntitiesManagerTest, UniqueTestId)
{
    constexpr size_t N = 10'000;
    PrefabEntity prefab = CreateTestIdPrefab();
    std::unordered_set<unsigned int> ids;
    std::vector<Entity> entities;

    for (size_t i = INVALID_ENTITY_ID + 1; i < N; ++i)
    {
        EntityWrapper entity = manager.Create(prefab);
        EXPECT_EQ(entity.getEntity().id, i);
        entities.push_back(entity.getEntity());

        entity.GetComponent<TestId>().id = i;
        ids.emplace(i);
    }
    EXPECT_EQ(ids.size(), manager.size());

    for (const Entity& entity : entities)
    {
        EXPECT_EQ(manager.GetComponent<TestId>(entity).id, entity.id);
        ids.erase(manager.GetComponent<TestId>(entity).id);
    }

    EXPECT_TRUE(ids.empty());
}


TEST_F(EntitiesManagerTest, DestroyEntityInMiddle_ComponentsRemainValid)
{
    std::vector<EntityWrapper> entities;

    for (int i = 0; i < 100; ++i)
    {
        auto prefab = Create2DPrefab(static_cast<float>(i), static_cast<float>(i));
        entities.emplace_back(manager.Create(prefab));
    }

    manager.Destroy(entities[50]);

    EXPECT_EQ(manager.size(), 99);

    for (int i = 0; i < 100; ++i)
    {
        if (i == 50)
            continue;

        EXPECT_TRUE(manager.IsAlive(entities[i]));

        const auto& pos = entities[i].GetComponent<Position2d>();
        EXPECT_FLOAT_EQ(pos.x, static_cast<float>(i));
        EXPECT_FLOAT_EQ(pos.y, static_cast<float>(i));
    }
}


TEST_F(EntitiesManagerTest, MassiveMiddleDeletion)
{
    std::vector<EntityWrapper> entities;

    for (int i = 0; i < 1000; ++i)
    {
        auto prefab = Create2DPrefab(static_cast<float>(i), 0.f);
        entities.emplace_back(manager.Create(prefab));
    }

    for (int i = 300; i < 700; ++i)
    {
        manager.Destroy(entities[i]);
    }

    EXPECT_EQ(manager.size(), 600);

    for (int i = 0; i < 1000; ++i)
    {
        if (i >= 300 && i < 700)
        {
            EXPECT_FALSE(manager.IsAlive(entities[i]));
        }
        else
        {
            EXPECT_TRUE(manager.IsAlive(entities[i]));

            const auto& pos = entities[i].GetComponent<Position2d>();
            EXPECT_FLOAT_EQ(pos.x, static_cast<float>(i));
        }
    }
}


TEST_F(EntitiesManagerTest, RemoveEverySecondEntity)
{
    std::vector<EntityWrapper> entities;

    for (int i = 0; i < 500; ++i)
    {
        entities.emplace_back(manager.Create(Create2DPrefab(static_cast<float>(i), 0.f)));
    }

    for (size_t i = 0; i < entities.size(); i += 2)
    {
        manager.Destroy(entities[i]);
    }

    EXPECT_EQ(manager.size(), 250);

    for (size_t i = 1; i < entities.size(); i += 2)
    {
        EXPECT_TRUE(manager.IsAlive(entities[i]));

        const auto& pos = entities[i].GetComponent<Position2d>();
        EXPECT_FLOAT_EQ(pos.x, static_cast<float>(i));
    }
}


TEST_F(EntitiesManagerTest, RemoveMiddleThenInsertAgain)
{
    std::vector<EntityWrapper> entities;

    for (int i = 0; i < 100; ++i)
    {
        entities.emplace_back(manager.Create(Create2DPrefab()));
    }

    Entity removed = entities[50].getEntity();

    manager.Destroy(entities[50]);

    auto newEntity = manager.Create(Create2DPrefab(999.f, 999.f));

    EXPECT_EQ(newEntity.getEntity().id, removed.id);
    EXPECT_EQ(newEntity.getEntity().version,
              removed.version + 1);

    const auto& pos = newEntity.GetComponent<Position2d>();

    EXPECT_FLOAT_EQ(pos.x, 999.f);
    EXPECT_FLOAT_EQ(pos.y, 999.f);
}


TEST_F(EntitiesManagerTest, ViewAfterManyRemovalsAndInsertions)
{
    std::vector<Entity> entities;

    for (int i = 0; i < 200; ++i)
    {
        entities.emplace_back(manager.Create(Create2DPrefab(static_cast<float>(i), 0.f)));
    }

    for (int i = 50; i < 150; ++i)
    {
        manager.Destroy(entities[i]);
    }

    for (int i = 0; i < 100; ++i)
    {
        manager.Create(Create2DPrefab(1000.f + i, 0.f));
    }

    size_t count = 0;

    for (auto [pos] : manager.view<Position2d>())
    {
        (void)pos;
        ++count;
    }

    EXPECT_EQ(count, manager.size());
}
