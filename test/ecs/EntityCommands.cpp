#include <gtest/gtest.h>
#include "ComponentsClass.hpp"
#include "ecs/Registry.hpp"
#include "ecs/registry/Commands.hpp"


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

    registry.Commands().Push(CreateEntityCmd{std::move(prefab), [&](Entity e) { created = e; }});

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

    registry.Commands().Push(CreateEntityCmd{std::move(prefab), [&](Entity e) {
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

    registry.Commands().Push(CreateEntityCmd{std::move(prefab)});

    registry.Update();
    EXPECT_EQ(registry.Entities().size(), 1);
}


TEST_F(EntityCommandsTest, CreateEntityFromSharedPtr)
{
    auto prefab = std::make_shared<PrefabEntity>();
    prefab->AddComponent<Position2d>();

    Entity created;
    registry.Commands().Push(CreateEntityCmd{prefab, [&](Entity e) { created = e; }});

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
    registry.Commands().Push(DeleteEntityCmd{entity, [&](Entity e) {
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
    registry.Commands().Push(DeleteEntityCmd{entity, [&](Entity) { ++callCount; }});

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

    registry.Commands().Push(CreateEntityCmd{std::move(prefab), [&](Entity e) {
        ++createCalls;
        created = e;
    }});

    EXPECT_EQ(registry.Entities().size(), 0);
    registry.Update();
    EXPECT_EQ(registry.Entities().size(), 1);
    EXPECT_TRUE(registry.Entities().IsAlive(created));

    registry.Commands().Push(DeleteEntityCmd{created, [&](Entity) { ++deleteCalls; }});

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

    registry.Commands().Push(CreateEntityCmd{std::move(prefab)});

    EXPECT_EQ(registry.Commands().size(), 1);
    registry.Update();
    EXPECT_EQ(registry.Commands().size(), 0);
}


TEST_F(EntityCommandsTest, CookCmdCreatesPlayerDeferred)
{
    registry.Commands().Push(CookCmd<Player, PlayerRecipe>{PlayerRecipe{}});
    EXPECT_EQ(registry.Entities().size(), 0);

    registry.Update();
    EXPECT_EQ(registry.Entities().size(), 1);

    int count = 0;
    for (auto [health, pos] : registry.Entities().view<Health, Position2d>())
    {
        EXPECT_EQ(health.value, 100.f);
        EXPECT_FLOAT_EQ(pos.x, 0.f);
        EXPECT_FLOAT_EQ(pos.y, 0.f);
        ++count;
    }
    EXPECT_EQ(count, 1);
}


TEST_F(EntityCommandsTest, CookCmdCreatesEnemyDeferred)
{
    registry.Commands().Push(CookCmd<Enemy, EnemyRecipe>{EnemyRecipe{}});
    registry.Update();

    int count = 0;
    for (auto [health, damage, pos] : registry.Entities().view<Health, Damage, Position2d>())
    {
        EXPECT_EQ(health.value, 50.f);
        EXPECT_EQ(damage.value, 15.f);
        EXPECT_FLOAT_EQ(pos.x, 10.f);
        EXPECT_FLOAT_EQ(pos.y, 10.f);
        ++count;
    }
    EXPECT_EQ(count, 1);
}


TEST_F(EntityCommandsTest, CookCmdCreatesProjectileDeferred)
{
    registry.Commands().Push(CookCmd<Projectile, ProjectileRecipe>{ProjectileRecipe{}});
    registry.Update();

    int count = 0;
    for (auto [pos, speed] : registry.Entities().view<Position3d, Speed>())
    {
        EXPECT_FLOAT_EQ(pos.x, 1.f);
        EXPECT_FLOAT_EQ(speed.value, 100.f);
        ++count;
    }
    EXPECT_EQ(count, 1);
}


TEST_F(EntityCommandsTest, CookCmdCreatesCameraDeferred)
{
    registry.Commands().Push(CookCmd<Camera, CameraRecipe>{});
    registry.Update();

    int count = 0;
    for (auto [pos, config] :  registry.Entities().view<Position3d, FullscreenConfig>())
    {
        EXPECT_FLOAT_EQ(pos.x, 0.f);
        EXPECT_FLOAT_EQ(pos.y, 0.f);
        EXPECT_FLOAT_EQ(pos.z, 0.f);
        EXPECT_TRUE(config.enabled);
        ++count;
    }
    EXPECT_EQ(count, 1);
}


TEST_F(EntityCommandsTest, CookCmdMultipleEntityTypes)
{
    registry.Commands().Push(CookCmd<Player, PlayerRecipe>{PlayerRecipe{}});
    registry.Commands().Push(CookCmd<Enemy, EnemyRecipe>{EnemyRecipe{}});
    registry.Commands().Push(CookCmd<Projectile, ProjectileRecipe>{ProjectileRecipe{}});
    registry.Commands().Push(CookCmd<Projectile, ProjectileRecipe>{ProjectileRecipe{}});
    registry.Commands().Push(CookCmd<Camera, CameraRecipe>{CameraRecipe{}});

    registry.Update();
    EXPECT_EQ(registry.Entities().size(), 5);


    int playerCount = 0;
    for (auto [health, pos] : registry.Entities().view<Health, Position2d>())
    {
        if (health.value > 50.f)
        {
            EXPECT_EQ(health.value, 100.f);
            ++playerCount;
        }
    }
    EXPECT_EQ(playerCount, 1);

    int enemyCount = 0;
    for (auto [health, damage, pos] : registry.Entities().view<Health, Damage, Position2d>())
    {
        EXPECT_EQ(damage.value, 15.f);
        ++enemyCount;
    }
    EXPECT_EQ(enemyCount, 1);

    int projCount = 0;
    for (auto [pos, speed] : registry.Entities().view<Position3d, Speed>())
    {
        EXPECT_FLOAT_EQ(speed.value, 100.f);
        ++projCount;
    }
    EXPECT_EQ(projCount, 2);
}


TEST_F(EntityCommandsTest, PlayerWrapperDirectCreation)
{
    PlayerRecipe recipe;
    PrefabEntity prefab;
    recipe.apply(prefab);

    Player player = registry.Entities().Create<Player>(std::move(prefab));

    EXPECT_TRUE(player.IsAlive());
    EXPECT_EQ(player.GetHealth(), 100.f);
    EXPECT_FLOAT_EQ(player.GetPosition().x, 0.f);
    EXPECT_FLOAT_EQ(player.GetPosition().y, 0.f);

    player.SetHealth(75.f);
    EXPECT_EQ(player.GetHealth(), 75.f);

    player.SelfDestroy();
    EXPECT_FALSE(player.IsAlive());
}


TEST_F(EntityCommandsTest, EnemyWrapperDirectCreation)
{
    EnemyRecipe recipe;
    PrefabEntity prefab;
    recipe.apply(prefab);

    Enemy enemy = registry.Entities().Create<Enemy>(std::move(prefab));

    EXPECT_TRUE(enemy.IsAlive());
    EXPECT_EQ(enemy.GetHealth(), 50.f);
    EXPECT_EQ(enemy.GetDamage(), 15.f);
    EXPECT_FLOAT_EQ(enemy.GetPosition().x, 10.f);
    EXPECT_FLOAT_EQ(enemy.GetPosition().y, 10.f);

    enemy.SelfDestroy();
    EXPECT_FALSE(enemy.IsAlive());
}


TEST_F(EntityCommandsTest, ProjectileWrapperDirectCreation)
{
    ProjectileRecipe recipe;
    PrefabEntity prefab;
    recipe.apply(prefab);

    Projectile proj = registry.Entities().Create<Projectile>(std::move(prefab));

    EXPECT_TRUE(proj.IsAlive());
    EXPECT_FLOAT_EQ(proj.GetPosition().x, 1.f);
    EXPECT_FLOAT_EQ(proj.GetPosition().y, 2.f);
    EXPECT_FLOAT_EQ(proj.GetPosition().z, 3.f);
    EXPECT_FLOAT_EQ(proj.GetSpeed(), 100.f);

    proj.SelfDestroy();
    EXPECT_FALSE(proj.IsAlive());
}


TEST_F(EntityCommandsTest, CameraWrapperDirectCreation)
{
    CameraRecipe recipe;
    PrefabEntity prefab;
    recipe.apply(prefab);

    Camera camera = registry.Entities().Create<Camera>(std::move(prefab));

    EXPECT_TRUE(camera.IsAlive());
    EXPECT_TRUE(camera.IsFullscreen());
    EXPECT_FLOAT_EQ(camera.GetPosition().x, 0.f);

    camera.SelfDestroy();
    EXPECT_FALSE(camera.IsAlive());
}


TEST_F(EntityCommandsTest, PlayerDeferredThenWrapAndMutate)
{
    Entity rawEntity;
    PrefabEntity prefab;
    PlayerRecipe{}.apply(prefab);

    registry.Commands().Push(CreateEntityCmd{std::move(prefab), [&](Entity e) {
        rawEntity = e;
    }});
    registry.Update();

    ASSERT_TRUE(registry.Entities().IsAlive(rawEntity));

    Player player{rawEntity, registry.Entities()};
    EXPECT_EQ(player.GetHealth(), 100.f);
    EXPECT_FLOAT_EQ(player.GetPosition().x, 0.f);

    player.SetHealth(42.f);
    EXPECT_EQ(player.GetHealth(), 42.f);

    EXPECT_EQ(player.GetComponent<Health>().value, 42.f);
}


TEST_F(EntityCommandsTest, EnemyDeferredThenWrap)
{
    Entity rawEntity;
    PrefabEntity prefab;
    EnemyRecipe{}.apply(prefab);

    registry.Commands().Push(CreateEntityCmd{std::move(prefab), [&](Entity e) {
        rawEntity = e;
    }});
    registry.Update();

    ASSERT_TRUE(registry.Entities().IsAlive(rawEntity));

    Enemy enemy{rawEntity, registry.Entities()};
    EXPECT_EQ(enemy.GetHealth(), 50.f);
    EXPECT_EQ(enemy.GetDamage(), 15.f);
    EXPECT_FLOAT_EQ(enemy.GetPosition().x, 10.f);
    EXPECT_FLOAT_EQ(enemy.GetPosition().y, 10.f);
}


TEST_F(EntityCommandsTest, EntityWrapperLikeSizeConstraint)
{
    static_assert(ecs::EntityWrapperLike<Player>,
        "Player must satisfy EntityWrapperLike (no data members)");
    static_assert(ecs::EntityWrapperLike<Enemy>,
        "Enemy must satisfy EntityWrapperLike (no data members)");
    static_assert(ecs::EntityWrapperLike<Projectile>,
        "Projectile must satisfy EntityWrapperLike (no data members)");
    static_assert(ecs::EntityWrapperLike<Camera>,
        "Camera must satisfy EntityWrapperLike (no data members)");
    static_assert(sizeof(Player) == sizeof(ecs::EntityWrapper),
        "Player must not add data members");
    static_assert(sizeof(Enemy) == sizeof(ecs::EntityWrapper),
        "Enemy must not add data members");
    static_assert(sizeof(Projectile) == sizeof(ecs::EntityWrapper),
        "Projectile must not add data members");
    static_assert(sizeof(Camera) == sizeof(ecs::EntityWrapper),
        "Camera must not add data members");
}
