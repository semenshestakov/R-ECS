#include <gtest/gtest.h>
#include "ecs/Registry.hpp"
#include "ecs/registry/Commands.hpp"
#include "ComponentsClass.hpp"


using namespace ecs;


class EntityCommandsTest : public ::testing::Test
{
protected:
    Registry registry;

    static PrefabEntity Create2DPrefab(float x = 1.f, float y = 2.f)
    {
        PrefabEntity prefab;
        prefab.AddComponent<Position2d>(x, y);
        return prefab;
    }

};


TEST_F(EntityCommandsTest, CreateEntityExecutesOnFlush)
{
    Entity created;
    PrefabEntity prefab;
    prefab.AddComponent<Position2d>(3.f, 4.f);

    registry.Commands().Push(CreateEntityCommand{std::move(prefab), registry, [&](Entity e) { created = e; }});

    EXPECT_FALSE(registry.Entities().IsAlive(created));
    EXPECT_EQ(registry.Commands().size(), 1);

    registry.Update();

    EXPECT_TRUE(registry.Entities().IsAlive(created));
    EXPECT_EQ(registry.Entities().size(), 1);
}


TEST_F(EntityCommandsTest, CreateEntityCallbackFired)
{
    int callCount = 0;
    PrefabEntity prefab;
    prefab.AddComponent<Position2d>();

    registry.Commands().Push(CreateEntityCommand{std::move(prefab), registry, [&](Entity e) {
        ++callCount;
        EXPECT_NE(e.id, INVALID_ENTITY_ID);
    }});

    registry.Update();
    EXPECT_EQ(callCount, 1);
}


TEST_F(EntityCommandsTest, CreateEntityNoCallback)
{
    PrefabEntity prefab;
    prefab.AddComponent<Position2d>();

    registry.Commands().Push(CreateEntityCommand{std::move(prefab), registry, nullptr});

    registry.Update();
    EXPECT_EQ(registry.Entities().size(), 1);
}


TEST_F(EntityCommandsTest, CreateEntityFromSharedPtr)
{
    auto prefab = std::make_shared<PrefabEntity>();
    prefab->AddComponent<Position2d>();

    Entity created;
    registry.Commands().Push(CreateEntityCommand{prefab, registry, [&](Entity e) { created = e; }});

    registry.Update();
    EXPECT_TRUE(registry.Entities().IsAlive(created));
    EXPECT_EQ(registry.Entities().size(), 1);
}


TEST_F(EntityCommandsTest, DeleteEntityDestroysOnFlush)
{
    auto wrapper = registry.Entities().Create(Create2DPrefab());
    const Entity entity = wrapper.getEntity();
    ASSERT_TRUE(registry.Entities().IsAlive(entity));

    int callCount = 0;
    registry.Commands().Push(DeleteEntityCommand{entity, registry, [&](Entity e) {
        ++callCount;
        EXPECT_TRUE(registry.Entities().IsAlive(e));
    }});

    registry.Update();

    EXPECT_FALSE(registry.Entities().IsAlive(entity));
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(registry.Entities().size(), 0);
}


TEST_F(EntityCommandsTest, DeleteDeadEntityIsSkipped)
{
    auto wrapper = registry.Entities().Create(Create2DPrefab());
    const Entity entity = wrapper.getEntity();

    registry.Entities().Destroy(entity);
    ASSERT_FALSE(registry.Entities().IsAlive(entity));

    int callCount = 0;
    registry.Commands().Push(DeleteEntityCommand{entity, registry, [&](Entity) { ++callCount; }});

    registry.Update();
    EXPECT_EQ(callCount, 0);
}


TEST_F(EntityCommandsTest, CreateThenDeleteInSameFlush)
{
    Entity created;
    int createCalls = 0;
    int deleteCalls = 0;

    PrefabEntity prefab;
    prefab.AddComponent<Position2d>();

    registry.Commands().Push(CreateEntityCommand{std::move(prefab), registry, [&](Entity e) {
        ++createCalls;
        created = e;
    }});

    EXPECT_EQ(registry.Entities().size(), 0);
    registry.Update();
    EXPECT_EQ(registry.Entities().size(), 1);
    EXPECT_TRUE(registry.Entities().IsAlive(created));

    registry.Commands().Push(DeleteEntityCommand{created, registry, [&](Entity) { ++deleteCalls; }});

    EXPECT_EQ(registry.Entities().size(), 1);
    registry.Update();
    EXPECT_EQ(registry.Entities().size(), 0);

    EXPECT_EQ(createCalls, 1);
    EXPECT_FALSE(registry.Entities().IsAlive(created));

    EXPECT_EQ(deleteCalls, 1);
    EXPECT_FALSE(registry.Entities().IsAlive(created));
}


TEST_F(EntityCommandsTest, CommandsEmptyAfterFlush)
{
    PrefabEntity prefab;
    prefab.AddComponent<Position2d>();

    registry.Commands().Push(CreateEntityCommand{std::move(prefab), registry, nullptr});

    EXPECT_EQ(registry.Commands().size(), 1);
    registry.Update();
    EXPECT_EQ(registry.Commands().size(), 0);
}
