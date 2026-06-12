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


TEST_F(EntitiesManagerTest, IsAlive_DefaultConstructedEntity_ReturnsFalse)
{
    Entity invalid{};
    EXPECT_FALSE(manager.IsAlive(invalid));
}


TEST_F(EntitiesManagerTest, TryGetComponent_ExistingComponent_ReturnsNonNull)
{
    auto entity = manager.Create(Create2DPrefab(3.f, 4.f));
    auto* pos = entity.TryGetComponent<Position2d>();

    ASSERT_NE(pos, nullptr);
    EXPECT_FLOAT_EQ(pos->x, 3.f);
    EXPECT_FLOAT_EQ(pos->y, 4.f);
}


TEST_F(EntitiesManagerTest, MutateComponent_ChangesPersist)
{
    auto entity = manager.Create(Create2DPrefab(1.f, 2.f));
    entity.GetComponent<Position2d>().x = 99.f;

    EXPECT_FLOAT_EQ(entity.GetComponent<Position2d>().x, 99.f);
    EXPECT_FLOAT_EQ(entity.GetComponent<Position2d>().y, 2.f);
}


TEST_F(EntitiesManagerTest, View_EmptyManager_NoIterations)
{
    size_t count = 0;
    for (auto [pos] : manager.view<Position2d>())
        ++count;

    EXPECT_EQ(count, 0);
}


TEST_F(EntitiesManagerTest, View_NoMatchingEntities_NoIterations)
{
    manager.Create(Create3DPrefab());

    size_t count = 0;
    for (auto [pos] : manager.view<Position2d>())
        ++count;

    EXPECT_EQ(count, 0);
}


TEST_F(EntitiesManagerTest, MutateComponent_ViaView_ChangesPersist)
{
    manager.Create(Create2DPrefab(1.f, 1.f));
    manager.Create(Create2DPrefab(2.f, 2.f));

    for (auto [pos] : manager.view<Position2d>())
        pos.x = 77.f;

    for (auto [pos] : manager.view<Position2d>())
        EXPECT_FLOAT_EQ(pos.x, 77.f);
}


TEST_F(EntitiesManagerTest, ViewAfterManyRemovalsAndInsertions)
{
    std::vector<Entity> entities;

    entities.reserve(200);
    for (int i = 0; i < 200; ++i)
    {
        entities.emplace_back(manager.Create<Entity>(Create2DPrefab(static_cast<float>(i), 0.f)));
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


TEST_F(EntitiesManagerTest, Create_Rvalue_InvokesMoveConstructor)
{
    MoveTracker::reset();

    PrefabEntity prefab;
    prefab.AddComponent<MoveTracker>(42);

    manager.Create(std::move(prefab));

    EXPECT_EQ(MoveTracker::moveCount, 1);
    EXPECT_EQ(MoveTracker::copyCount, 0);
}


TEST_F(EntitiesManagerTest, Create_Lvalue_InvokesCopyConstructor)
{
    MoveTracker::reset();

    PrefabEntity prefab;
    prefab.AddComponent<MoveTracker>(42);

    manager.Create(prefab);

    EXPECT_EQ(MoveTracker::copyCount, 1);
    EXPECT_EQ(MoveTracker::moveCount, 0);
}


TEST_F(EntitiesManagerTest, Create_Rvalue_ComponentDataCorrect)
{
    PrefabEntity prefab;
    prefab.AddComponent<MoveTracker>(42);

    const auto entity = manager.Create(std::move(prefab));

    EXPECT_EQ(entity.GetComponent<MoveTracker>().value, 42);
}


TEST_F(EntitiesManagerTest, Create_Lvalue_ComponentDataCorrect)
{
    PrefabEntity prefab;
    prefab.AddComponent<MoveTracker>(42);

    const auto entity = manager.Create(prefab);

    EXPECT_EQ(entity.GetComponent<MoveTracker>().value, 42);
    const auto entity2 = manager.Create(prefab);
    EXPECT_EQ(entity2.GetComponent<MoveTracker>().value, 42);
}


TEST_F(EntitiesManagerTest, Create_Rvalue_SourceIsMovedFrom)
{
    MoveTracker::reset();

    PrefabEntity prefab;
    prefab.AddComponent<MoveTracker>(99);

    manager.Create(std::move(prefab));

    EXPECT_EQ(MoveTracker::moveCount, 1);
    EXPECT_EQ(MoveTracker::copyCount, 0);
}


TEST_F(EntitiesManagerTest, Create_Rvalue_Position3d_NoExtraAllocation)
{
    PrefabEntity prefab;
    prefab.AddComponent<Position3d>(5.f, 6.f, 7.f);

    const auto entity = manager.Create(std::move(prefab));

    const auto& pos = entity.GetComponent<Position3d>();
    EXPECT_NE(pos.testLeaks, nullptr);
    EXPECT_FLOAT_EQ(pos.x, 5.f);
    EXPECT_FLOAT_EQ(pos.y, 6.f);
    EXPECT_FLOAT_EQ(pos.z, 7.f);
}


TEST_F(EntitiesManagerTest, Create_Lvalue_Position3d_CopyLeavesTestLeaksNull)
{
    PrefabEntity prefab;
    prefab.AddComponent<Position3d>(1.f, 2.f, 3.f);

    const auto entity = manager.Create(prefab);

    const auto& pos = entity.GetComponent<Position3d>();
    EXPECT_EQ(pos.testLeaks, nullptr);
    EXPECT_FLOAT_EQ(pos.x, 1.f);
    EXPECT_FLOAT_EQ(pos.y, 2.f);
    EXPECT_FLOAT_EQ(pos.z, 3.f);
}


TEST_F(EntitiesManagerTest, Create_Rvalue_TemporaryPrefab)
{
    MoveTracker::reset();

    auto makePrefab = [](int v)
    {
        PrefabEntity p;
        p.AddComponent<MoveTracker>(v);
        return p;
    };

    const auto entity = manager.Create(makePrefab(7));

    EXPECT_EQ(MoveTracker::moveCount, 1);
    EXPECT_EQ(MoveTracker::copyCount, 0);
    EXPECT_EQ(entity.GetComponent<MoveTracker>().value, 7);
}


TEST_F(EntitiesManagerTest, Create_Rvalue_MultipleComponents_AllMoved)
{
    MoveTracker::reset();

    PrefabEntity prefab;
    prefab.AddComponent<MoveTracker>(10);
    prefab.AddComponent<Position2d>(3.f, 4.f);

    const auto entity = manager.Create(std::move(prefab));

    EXPECT_EQ(MoveTracker::moveCount, 1);
    EXPECT_EQ(MoveTracker::copyCount, 0);
    EXPECT_EQ(entity.GetComponent<MoveTracker>().value, 10);
    EXPECT_FLOAT_EQ(entity.GetComponent<Position2d>().x, 3.f);
}


TEST_F(EntitiesManagerTest, Destroy_InvalidEntity_NoOp)
{
    const Entity invalid{};
    manager.Destroy(invalid);
    EXPECT_EQ(manager.size(), 0);
}


TEST_F(EntitiesManagerTest, Destroy_AlreadyDestroyed_NoDoubleFree)
{
    const auto entity = manager.Create<Entity>(Create2DPrefab());

    manager.Destroy(entity);
    manager.Destroy(entity);

    EXPECT_FALSE(manager.IsAlive(entity));
    EXPECT_EQ(manager.size(), 0);
}


TEST_F(EntitiesManagerTest, Destroy_LastEntity_NoRelocation)
{
    const auto a = manager.Create<Entity>(Create2DPrefab(1.f, 1.f));
    const auto b = manager.Create<Entity>(Create2DPrefab(2.f, 2.f));

    manager.Destroy(b);

    EXPECT_TRUE(manager.IsAlive(a));
    EXPECT_FALSE(manager.IsAlive(b));
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(a).x, 1.f);
}


TEST_F(EntitiesManagerTest, GetComponentData_RawAccess_MatchesTypedAccess)
{
    const auto entity = manager.Create<Entity>(Create2DPrefab(3.f, 4.f));

    byte* raw = manager.GetComponentData(entity, ComponentRegistrator::GetСomponentId<Position2d>());
    ASSERT_NE(raw, nullptr);

    const auto* pos = std::bit_cast<Position2d*>(raw);
    EXPECT_FLOAT_EQ(pos->x, 3.f);
    EXPECT_FLOAT_EQ(pos->y, 4.f);

    EXPECT_EQ(manager.GetComponentData(Entity{}, ComponentRegistrator::GetСomponentId<Position2d>()), nullptr);
}


TEST_F(EntitiesManagerTest, AddComponents_NewComponent_MigratesArchetype)
{
    auto prefab = Create2DPrefab(5.f, 6.f);
    const auto entity = manager.Create<Entity>(prefab);

    manager.AddComponents(entity, Position3d{7.f, 8.f, 9.f});

    EXPECT_TRUE(manager.IsAlive(entity));
    EXPECT_EQ(manager.size(), 1);

    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).x, 5.f);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).y, 6.f);

    const auto& pos3d = manager.GetComponent<Position3d>(entity);
    EXPECT_FLOAT_EQ(pos3d.x, 7.f);
    EXPECT_FLOAT_EQ(pos3d.y, 8.f);
    EXPECT_FLOAT_EQ(pos3d.z, 9.f);
}


TEST_F(EntitiesManagerTest, AddComponents_NewComponent_Rvalue_MoveConstructs)
{
    const auto entity = manager.Create<Entity>(Create2DPrefab());

    LifeStats::reset();
    manager.AddComponents(entity, LifeTracker{42});

    EXPECT_EQ(LifeStats::moveCtor, 1);
    EXPECT_EQ(LifeStats::copyCtor, 0);
    EXPECT_EQ(LifeStats::copyAssign, 0);
    EXPECT_EQ(LifeStats::moveAssign, 0);
    EXPECT_EQ(manager.GetComponent<LifeTracker>(entity).value, 42);
}


TEST_F(EntitiesManagerTest, AddComponents_NewComponent_Lvalue_CopyConstructs)
{
    const auto entity = manager.Create<Entity>(Create2DPrefab());

    LifeTracker source{42};
    LifeStats::reset();
    manager.AddComponents(entity, source);

    EXPECT_EQ(LifeStats::copyCtor, 1);
    EXPECT_EQ(LifeStats::moveCtor, 0);
    EXPECT_EQ(LifeStats::copyAssign, 0);
    EXPECT_EQ(LifeStats::moveAssign, 0);
    EXPECT_EQ(manager.GetComponent<LifeTracker>(entity).value, 42);
}


TEST_F(EntitiesManagerTest, AddComponents_ExistingComponent_NoMigration_OverwritesInPlace)
{
    PrefabEntity prefab;
    prefab.AddComponent<LifeTracker>(1);
    const auto entity = manager.Create<Entity>(prefab);

    LifeStats::reset();
    manager.AddComponents(entity, LifeTracker{99});

    EXPECT_EQ(LifeStats::moveCtor, 0);
    EXPECT_EQ(LifeStats::copyCtor, 0);
    EXPECT_EQ(LifeStats::moveAssign, 1);
    EXPECT_EQ(LifeStats::copyAssign, 0);
    EXPECT_EQ(manager.GetComponent<LifeTracker>(entity).value, 99);
    EXPECT_EQ(manager.size(), 1);
}


TEST_F(EntitiesManagerTest, AddComponents_ExistingComponent_Lvalue_CopyAssignsInPlace)
{
    PrefabEntity prefab;
    prefab.AddComponent<LifeTracker>(1);
    const auto entity = manager.Create<Entity>(prefab);

    LifeTracker source{77};
    LifeStats::reset();
    manager.AddComponents(entity, source);

    EXPECT_EQ(LifeStats::copyAssign, 1);
    EXPECT_EQ(LifeStats::moveAssign, 0);
    EXPECT_EQ(LifeStats::moveCtor, 0);
    EXPECT_EQ(LifeStats::copyCtor, 0);
    EXPECT_EQ(manager.GetComponent<LifeTracker>(entity).value, 77);
}


TEST_F(EntitiesManagerTest, AddComponents_PreservesExistingTrackedComponent_DuringMigration)
{
    PrefabEntity prefab;
    prefab.AddComponent<LifeTracker>(5);
    const auto entity = manager.Create<Entity>(prefab);

    LifeStats::reset();
    manager.AddComponents(entity, Position2d{1.f, 2.f});

    EXPECT_EQ(LifeStats::moveCtor, 1);
    EXPECT_EQ(LifeStats::copyCtor, 0);
    EXPECT_EQ(LifeStats::dtor, 1);
    EXPECT_EQ(manager.GetComponent<LifeTracker>(entity).value, 5);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).x, 1.f);
}


TEST_F(EntitiesManagerTest, AddComponents_MixedNewAndExisting_DuringMigration)
{
    PrefabEntity prefab;
    prefab.AddComponent<LifeTracker>(1);
    prefab.AddComponent<Position2d>(0.f, 0.f);
    const auto entity = manager.Create<Entity>(prefab);

    LifeStats::reset();
    manager.AddComponents(entity, LifeTracker{9}, Position3d{1.f, 2.f, 3.f});

    EXPECT_EQ(LifeStats::moveCtor, 1);
    EXPECT_EQ(LifeStats::moveAssign, 1);
    EXPECT_EQ(LifeStats::copyAssign, 0);
    EXPECT_EQ(manager.GetComponent<LifeTracker>(entity).value, 9);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).x, 0.f);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position3d>(entity).z, 3.f);
}


TEST_F(EntitiesManagerTest, AddComponents_MultipleNewComponents_AtOnce)
{
    const auto entity = manager.Create<Entity>(CreateTestIdPrefab());

    manager.AddComponents(entity, Position2d{1.f, 2.f}, Position3d{4.f, 5.f, 6.f});

    EXPECT_NE(manager.TryGetComponent<TestId>(entity), nullptr);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).x, 1.f);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position3d>(entity).z, 6.f);
}


TEST_F(EntitiesManagerTest, AddComponents_DeadEntity_NoOp)
{
    const Entity invalid{999, 999};
    manager.AddComponents(invalid, Position3d{});
    EXPECT_EQ(manager.size(), 0);

    const auto entity = manager.Create<Entity>(Create2DPrefab());
    manager.Destroy(entity);

    manager.AddComponents(entity, Position3d{});
    EXPECT_FALSE(manager.IsAlive(entity));
    EXPECT_EQ(manager.size(), 0);
}


TEST_F(EntitiesManagerTest, AddComponents_MigrationSwapRemove_KeepsOtherEntitiesValid)
{
    const auto a = manager.Create<Entity>(Create2DPrefab(1.f, 10.f));
    const auto b = manager.Create<Entity>(Create2DPrefab(2.f, 20.f));
    const auto c = manager.Create<Entity>(Create2DPrefab(3.f, 30.f));

    manager.AddComponents(b, Position3d{0.f, 0.f, 99.f});

    EXPECT_TRUE(manager.IsAlive(a));
    EXPECT_TRUE(manager.IsAlive(b));
    EXPECT_TRUE(manager.IsAlive(c));
    EXPECT_EQ(manager.size(), 3);

    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(a).x, 1.f);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(b).x, 2.f);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(c).x, 3.f);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(c).y, 30.f);

    EXPECT_FLOAT_EQ(manager.GetComponent<Position3d>(b).z, 99.f);
    EXPECT_EQ(manager.TryGetComponent<Position3d>(a), nullptr);
    EXPECT_EQ(manager.TryGetComponent<Position3d>(c), nullptr);
}


TEST_F(EntitiesManagerTest, AddComponents_ReusesExistingTargetArchetype)
{
    const auto e1 = manager.Create<Entity>(Create2DPrefab(1.f, 1.f));
    manager.AddComponents(e1, Position3d{1.f, 1.f, 1.f});

    const auto e2 = manager.Create<Entity>(CreateMixedPrefab());

    size_t count = 0;
    for (auto [p2d, p3d] : manager.view<Position2d, Position3d>())
    {
        (void)p2d; (void)p3d;
        ++count;
    }
    EXPECT_EQ(count, 2);
    EXPECT_TRUE(manager.IsAlive(e1));
    EXPECT_TRUE(manager.IsAlive(e2));
}


TEST_F(EntitiesManagerTest, AddComponents_ThenView_SeesEntityInBothViews)
{
    const auto entity = manager.Create<Entity>(Create2DPrefab(4.f, 4.f));
    manager.AddComponents(entity, Position3d{});

    size_t oldView = 0;
    for (auto [p] : manager.view<Position2d>()) { (void)p; ++oldView; }

    size_t newView = 0;
    for (auto [p] : manager.view<Position3d>()) { (void)p; ++newView; }

    EXPECT_EQ(oldView, 1);
    EXPECT_EQ(newView, 1);
}


TEST_F(EntitiesManagerTest, AddComponents_RepeatedMigrations_GrowingArchetype)
{
    const auto entity = manager.Create<Entity>(CreateTestIdPrefab());
    manager.GetComponent<TestId>(entity).id = 7;

    manager.AddComponents(entity, PositionX{11.f});
    manager.AddComponents(entity, PositionY{22.f});
    manager.AddComponents(entity, PositionZ{33.f});

    EXPECT_EQ(manager.GetComponent<TestId>(entity).id, 7u);
    EXPECT_FLOAT_EQ(manager.GetComponent<PositionX>(entity).x, 11.f);
    EXPECT_FLOAT_EQ(manager.GetComponent<PositionY>(entity).y, 22.f);
    EXPECT_FLOAT_EQ(manager.GetComponent<PositionZ>(entity).z, 33.f);
    EXPECT_EQ(manager.size(), 1);
}


TEST_F(EntitiesManagerTest, AddComponents_OverwriteOnly_KeepsArchetypeAndLocation)
{
    auto prefab = CreateMixedPrefab();
    const auto entity = manager.Create<Entity>(prefab);

    manager.AddComponents(entity, Position2d{1.f, 2.f}, Position3d{3.f, 4.f, 5.f});

    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).x, 1.f);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).y, 2.f);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position3d>(entity).z, 5.f);
    EXPECT_EQ(manager.size(), 1);
}


TEST_F(EntitiesManagerTest, AddComponents_Position3d_NoLeakAcrossMigration)
{
    const auto a = manager.Create<Entity>(Create2DPrefab());
    manager.AddComponents(a, Position3d{1.f, 2.f, 3.f});
    EXPECT_NE(manager.GetComponent<Position3d>(a).testLeaks, nullptr);

    const auto b = manager.Create<Entity>(Create2DPrefab());
    Position3d src{4.f, 5.f, 6.f};
    manager.AddComponents(b, src);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position3d>(b).z, 6.f);

    EXPECT_FLOAT_EQ(manager.GetComponent<Position3d>(a).z, 3.f);
}


TEST_F(EntitiesManagerTest, AddComponents_ManyEntities_DataIntegrity)
{
    constexpr int N = 200;
    std::vector<Entity> entities;
    entities.reserve(N);

    for (int i = 0; i < N; ++i)
        entities.emplace_back(manager.Create<Entity>(Create2DPrefab(static_cast<float>(i), 0.f)));

    for (int i = 50; i < 150; ++i)
        manager.AddComponents(entities[i], Position3d{0.f, 0.f, static_cast<float>(i)});

    EXPECT_EQ(manager.size(), static_cast<size_t>(N));

    for (int i = 0; i < N; ++i)
    {
        ASSERT_TRUE(manager.IsAlive(entities[i]));
        EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entities[i]).x, static_cast<float>(i));

        if (i >= 50 && i < 150)
        {
            ASSERT_NE(manager.TryGetComponent<Position3d>(entities[i]), nullptr);
            EXPECT_FLOAT_EQ(manager.GetComponent<Position3d>(entities[i]).z, static_cast<float>(i));
        }
        else
        {
            EXPECT_EQ(manager.TryGetComponent<Position3d>(entities[i]), nullptr);
        }
    }

    size_t migrated = 0;
    for (auto [p2d, p3d] : manager.view<Position2d, Position3d>()) { (void)p2d; (void)p3d; ++migrated; }
    EXPECT_EQ(migrated, 100u);
}


TEST_F(EntitiesManagerTest, AddComponents_AllComponentsBalanced_NoDanglingState)
{
    LifeStats::reset();
    {
        EntitiesManager local;
        const auto e = local.Create<Entity>(Create2DPrefab());
        local.AddComponents(e, LifeTracker{1});
        local.AddComponents(e, LifeTracker{2});
        local.AddComponents(e, Position3d{}, LifeTracker{3});
        EXPECT_EQ(local.GetComponent<LifeTracker>(e).value, 3);
    }

    EXPECT_LE(LifeStats::dtor, LifeStats::liveConstructions());
    EXPECT_GT(LifeStats::dtor, 0);
}


TEST_F(EntitiesManagerTest, RemoveComponents_RemovesComponent_MigratesArchetype)
{
    const auto entity = manager.Create<Entity>(CreateMixedPrefab()); // {Position2d, Position3d}

    manager.RemoveComponents<Position3d>(entity);

    EXPECT_TRUE(manager.IsAlive(entity));
    EXPECT_EQ(manager.size(), 1);
    EXPECT_EQ(manager.TryGetComponent<Position3d>(entity), nullptr);

    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).x, 10.f);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).y, 20.f);
}


TEST_F(EntitiesManagerTest, RemoveComponents_NonexistentComponent_NoOp)
{
    const auto entity = manager.Create<Entity>(Create2DPrefab(3.f, 4.f));

    manager.RemoveComponents<Position3d>(entity);

    EXPECT_TRUE(manager.IsAlive(entity));
    EXPECT_EQ(manager.size(), 1);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).x, 3.f);
}


TEST_F(EntitiesManagerTest, RemoveComponents_MultipleAtOnce)
{
    PrefabEntity prefab;
    prefab.AddComponent<Position2d>(1.f, 2.f);
    prefab.AddComponent<Position3d>();
    prefab.AddComponent<TestId>();
    const auto entity = manager.Create<Entity>(prefab);

    manager.RemoveComponents<Position3d, TestId>(entity);

    EXPECT_NE(manager.TryGetComponent<Position2d>(entity), nullptr);
    EXPECT_EQ(manager.TryGetComponent<Position3d>(entity), nullptr);
    EXPECT_EQ(manager.TryGetComponent<TestId>(entity), nullptr);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).x, 1.f);
}


TEST_F(EntitiesManagerTest, RemoveComponents_PartiallyPresent_RemovesOnlyExisting)
{
    PrefabEntity prefab;
    prefab.AddComponent<Position2d>(5.f, 6.f);
    prefab.AddComponent<TestId>();
    const auto entity = manager.Create<Entity>(prefab);

    manager.RemoveComponents<Position3d, TestId>(entity);

    EXPECT_NE(manager.TryGetComponent<Position2d>(entity), nullptr);
    EXPECT_EQ(manager.TryGetComponent<TestId>(entity), nullptr);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).y, 6.f);
}


TEST_F(EntitiesManagerTest, RemoveComponents_LastComponent_DestroysEntity)
{
    const auto entity = manager.Create<Entity>(Create2DPrefab());

    manager.RemoveComponents<Position2d>(entity);

    EXPECT_FALSE(manager.IsAlive(entity));
    EXPECT_EQ(manager.size(), 0);
}


TEST_F(EntitiesManagerTest, RemoveComponents_AllListedComponents_DestroysEntity)
{
    const auto entity = manager.Create<Entity>(CreateMixedPrefab());

    manager.RemoveComponents<Position2d, Position3d>(entity);

    EXPECT_FALSE(manager.IsAlive(entity));
    EXPECT_EQ(manager.size(), 0);
}


TEST_F(EntitiesManagerTest, RemoveComponents_DeadEntity_NoOp)
{
    const Entity invalid{999, 999};
    manager.RemoveComponents<Position2d>(invalid);
    EXPECT_EQ(manager.size(), 0);

    const auto entity = manager.Create<Entity>(Create2DPrefab());
    manager.Destroy(entity);

    manager.RemoveComponents<Position2d>(entity);
    EXPECT_FALSE(manager.IsAlive(entity));
    EXPECT_EQ(manager.size(), 0);
}


TEST_F(EntitiesManagerTest, RemoveComponents_DestructsRemovedComponent)
{
    PrefabEntity prefab;
    prefab.AddComponent<LifeTracker>(5);
    prefab.AddComponent<Position2d>(0.f, 0.f);
    const auto entity = manager.Create<Entity>(prefab);

    LifeStats::reset();
    manager.RemoveComponents<LifeTracker>(entity);

    EXPECT_EQ(LifeStats::dtor, 1);
    EXPECT_EQ(LifeStats::moveCtor, 0);
    EXPECT_EQ(manager.TryGetComponent<LifeTracker>(entity), nullptr);
    EXPECT_NE(manager.TryGetComponent<Position2d>(entity), nullptr);
}


TEST_F(EntitiesManagerTest, RemoveComponents_SwapRemove_KeepsOtherEntitiesValid)
{
    std::vector<Entity> entities;
    for (int i = 0; i < 3; ++i)
    {
        PrefabEntity prefab;
        prefab.AddComponent<Position2d>(static_cast<float>(i), 0.f);
        prefab.AddComponent<Position3d>(0.f, 0.f, static_cast<float>(i));
        entities.emplace_back(manager.Create<Entity>(prefab));
    }

    manager.RemoveComponents<Position3d>(entities[1]);

    EXPECT_EQ(manager.size(), 3);
    for (int i = 0; i < 3; ++i)
    {
        ASSERT_TRUE(manager.IsAlive(entities[i]));
        EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entities[i]).x, static_cast<float>(i));
    }
    EXPECT_EQ(manager.TryGetComponent<Position3d>(entities[1]), nullptr);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position3d>(entities[0]).z, 0.f);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position3d>(entities[2]).z, 2.f);
}


TEST_F(EntitiesManagerTest, RemoveComponents_ThenView)
{
    manager.Create<Entity>(CreateMixedPrefab());
    const auto entity = manager.Create<Entity>(CreateMixedPrefab());

    manager.RemoveComponents<Position3d>(entity);

    size_t both = 0;
    for (auto [p2d, p3d] : manager.view<Position2d, Position3d>()) { (void)p2d; (void)p3d; ++both; }
    EXPECT_EQ(both, 1);

    size_t twoD = 0;
    for (auto [p2d] : manager.view<Position2d>()) { (void)p2d; ++twoD; }
    EXPECT_EQ(twoD, 2);
}


TEST_F(EntitiesManagerTest, RemoveComponents_HeapComponent_FreesBuffer)
{
    const auto entity = manager.Create<Entity>(CreateMixedPrefab());

    manager.RemoveComponents<Position3d>(entity);

    EXPECT_EQ(manager.TryGetComponent<Position3d>(entity), nullptr);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).x, 10.f);

    manager.Destroy(entity);
    EXPECT_EQ(manager.size(), 0);
}


TEST_F(EntitiesManagerTest, AddThenRemove_RoundTripsArchetype)
{
    const auto entity = manager.Create<Entity>(Create2DPrefab(7.f, 8.f));

    manager.AddComponents(entity, Position3d{1.f, 2.f, 3.f});
    EXPECT_NE(manager.TryGetComponent<Position3d>(entity), nullptr);

    manager.RemoveComponents<Position3d>(entity);

    EXPECT_EQ(manager.TryGetComponent<Position3d>(entity), nullptr);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).x, 7.f);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).y, 8.f);

    const auto other = manager.Create<Entity>(CreateMixedPrefab());
    manager.AddComponents(entity, Position3d{4.f, 5.f, 6.f});

    size_t count = 0;
    for (auto [p2d, p3d] : manager.view<Position2d, Position3d>()) { (void)p2d; (void)p3d; ++count; }
    EXPECT_EQ(count, 2);
    EXPECT_TRUE(manager.IsAlive(other));
}


TEST_F(EntitiesManagerTest, RemoveComponents_ManyEntities_DataIntegrity)
{
    constexpr int N = 200;
    std::vector<Entity> entities;
    entities.reserve(N);

    for (int i = 0; i < N; ++i)
    {
        PrefabEntity prefab;
        prefab.AddComponent<Position2d>(static_cast<float>(i), 0.f);
        prefab.AddComponent<TestId>();
        entities.emplace_back(manager.Create<Entity>(prefab));
        manager.GetComponent<TestId>(entities[i]).id = static_cast<unsigned int>(i);
    }

    for (int i = 50; i < 150; ++i)
        manager.RemoveComponents<TestId>(entities[i]);

    EXPECT_EQ(manager.size(), static_cast<size_t>(N));

    for (int i = 0; i < N; ++i)
    {
        ASSERT_TRUE(manager.IsAlive(entities[i]));
        EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entities[i]).x, static_cast<float>(i));

        if (i >= 50 && i < 150)
            EXPECT_EQ(manager.TryGetComponent<TestId>(entities[i]), nullptr);
        else
        {
            ASSERT_NE(manager.TryGetComponent<TestId>(entities[i]), nullptr);
            EXPECT_EQ(manager.GetComponent<TestId>(entities[i]).id, static_cast<unsigned int>(i));
        }
    }
}


TEST_F(EntitiesManagerTest, AddComponents_PreservedComponent_IsMovedOnce_NeverCopied)
{
    PrefabEntity prefab;
    prefab.AddComponent<MoveTracker>(7);
    const auto entity = manager.Create<Entity>(prefab);

    MoveTracker::reset();
    manager.AddComponents(entity, Position2d{1.f, 2.f});

    EXPECT_EQ(MoveTracker::moveCount, 1);
    EXPECT_EQ(MoveTracker::copyCount, 0);
    EXPECT_EQ(manager.GetComponent<MoveTracker>(entity).value, 7);
}


TEST_F(EntitiesManagerTest, AddComponents_MultiplePreservedComponents_EachMovedOnce)
{
    PrefabEntity prefab;
    prefab.AddComponent<MoveTracker>(1);
    prefab.AddComponent<Position2d>(3.f, 4.f);
    const auto entity = manager.Create<Entity>(prefab);

    MoveTracker::reset();
    manager.AddComponents(entity, Position3d{5.f, 6.f, 7.f});

    EXPECT_EQ(MoveTracker::moveCount, 1);
    EXPECT_EQ(MoveTracker::copyCount, 0);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).x, 3.f);
}


TEST_F(EntitiesManagerTest, AddComponents_NewComponent_Rvalue_OneMove_NoCopy)
{
    const auto entity = manager.Create<Entity>(Create2DPrefab());

    LifeStats::reset();
    manager.AddComponents(entity, LifeTracker{42});

    EXPECT_EQ(LifeStats::moveCtor, 1);
    EXPECT_EQ(LifeStats::copyCtor, 0);
    EXPECT_EQ(LifeStats::copyAssign, 0);
    EXPECT_EQ(LifeStats::moveAssign, 0);
    EXPECT_EQ(manager.GetComponent<LifeTracker>(entity).value, 42);
}


TEST_F(EntitiesManagerTest, AddComponents_NewComponent_Lvalue_OneCopy_NoMove)
{
    const auto entity = manager.Create<Entity>(Create2DPrefab());

    LifeTracker source{42};
    LifeStats::reset();
    manager.AddComponents(entity, source);

    EXPECT_EQ(LifeStats::copyCtor, 1);
    EXPECT_EQ(LifeStats::moveCtor, 0);
    EXPECT_EQ(LifeStats::copyAssign, 0);
    EXPECT_EQ(LifeStats::moveAssign, 0);
}


TEST_F(EntitiesManagerTest, AddComponents_OverwriteExisting_NoConstructors_OnlyAssignment)
{
    PrefabEntity prefab;
    prefab.AddComponent<LifeTracker>(1);
    const auto entity = manager.Create<Entity>(prefab);

    LifeStats::reset();
    manager.AddComponents(entity, LifeTracker{9});

    EXPECT_EQ(LifeStats::moveCtor, 0);
    EXPECT_EQ(LifeStats::copyCtor, 0);
    EXPECT_EQ(LifeStats::moveAssign, 1);
    EXPECT_EQ(LifeStats::copyAssign, 0);
}


TEST_F(EntitiesManagerTest, RemoveComponents_PreservedComponent_IsMovedOnce_NeverCopied)
{
    PrefabEntity prefab;
    prefab.AddComponent<MoveTracker>(5);
    prefab.AddComponent<Position2d>(1.f, 2.f);
    const auto entity = manager.Create<Entity>(prefab);

    MoveTracker::reset();
    manager.RemoveComponents<Position2d>(entity);

    EXPECT_EQ(MoveTracker::moveCount, 1);
    EXPECT_EQ(MoveTracker::copyCount, 0);
    EXPECT_EQ(manager.GetComponent<MoveTracker>(entity).value, 5);
}


TEST_F(EntitiesManagerTest, RemoveComponents_DroppedComponent_NeverCopiedOrMoved)
{
    PrefabEntity prefab;
    prefab.AddComponent<MoveTracker>(3);
    prefab.AddComponent<Position2d>();
    const auto entity = manager.Create<Entity>(prefab);

    MoveTracker::reset();
    manager.RemoveComponents<MoveTracker>(entity);

    EXPECT_EQ(MoveTracker::moveCount, 0);
    EXPECT_EQ(MoveTracker::copyCount, 0);
    EXPECT_EQ(manager.TryGetComponent<MoveTracker>(entity), nullptr);
}


TEST_F(EntitiesManagerTest, RemoveComponents_MultiplePreservedComponents_EachMovedOnce)
{
    PrefabEntity prefab;
    prefab.AddComponent<MoveTracker>(8);
    prefab.AddComponent<Position2d>(1.f, 1.f);
    prefab.AddComponent<TestId>();
    const auto entity = manager.Create<Entity>(prefab);

    MoveTracker::reset();
    manager.RemoveComponents<TestId>(entity);

    EXPECT_EQ(MoveTracker::moveCount, 1);
    EXPECT_EQ(MoveTracker::copyCount, 0);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(entity).x, 1.f);
}


TEST_F(EntitiesManagerTest, AddThenRemove_PreservedComponent_MovedOncePerMigration_NeverCopied)
{
    PrefabEntity prefab;
    prefab.AddComponent<MoveTracker>(11);
    const auto entity = manager.Create<Entity>(prefab);

    MoveTracker::reset();
    manager.AddComponents(entity, Position2d{1.f, 2.f});
    manager.RemoveComponents<Position2d>(entity);

    EXPECT_EQ(MoveTracker::moveCount, 2);
    EXPECT_EQ(MoveTracker::copyCount, 0);
    EXPECT_EQ(manager.GetComponent<MoveTracker>(entity).value, 11);
}


TEST_F(EntitiesManagerTest, RemoveComponents_NonexistentComponent_NoConstructorsAtAll)
{
    PrefabEntity prefab;
    prefab.AddComponent<MoveTracker>(4);
    const auto entity = manager.Create<Entity>(prefab);

    MoveTracker::reset();
    manager.RemoveComponents<Position2d>(entity);

    EXPECT_EQ(MoveTracker::moveCount, 0);
    EXPECT_EQ(MoveTracker::copyCount, 0);
    EXPECT_EQ(manager.GetComponent<MoveTracker>(entity).value, 4);
}
