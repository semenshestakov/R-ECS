#include <gtest/gtest.h>
#include "ecs/entities/EntitiesManager.hpp"

#include <set>
#include <type_traits>

#include "ComponentsClass.hpp"
#include "ecs/components/ComponentRegistrator.hpp"
#include "ecs/entities/PrefabEntity.hpp"


using namespace ecs;

// EntityWrapper derives from ecs::Tag, so every *named* wrapper subclass is itself a
// tag (filters on its own bit); only the base EntityWrapper is carved out of IsTag and
// stays the non-filtering handle. These assertions lock that contract in.
static_assert(IsTag<Frozen> && !EntityWrapperLike<Frozen>, "Frozen must be a plain tag");
static_assert(IsTag<Door> && EntityWrapperLike<Door>, "Door is a named wrapper -> wrapper-tag");
static_assert(IsTag<Player> && EntityWrapperLike<Player>, "Player is a named wrapper -> wrapper-tag");
static_assert(EntityWrapperLike<EntityWrapper> && !IsTag<EntityWrapper>,
              "base EntityWrapper is the only non-filtering wrapper handle");
static_assert(sizeof(Door) == sizeof(EntityWrapper) && sizeof(Player) == sizeof(EntityWrapper),
              "wrappers must not add data members");


class TagsTest : public ::testing::Test
{
protected:
    EntitiesManager manager;

    // Position2d, optionally carrying the Frozen and/or Hidden tags.
    static PrefabEntity FrozenPrefab(const float x = 1.f, const float y = 2.f,
                                     const bool frozen = true, const bool hidden = false)
    {
        PrefabEntity prefab;
        prefab.AddComponent<Position2d>(x, y);
        if (frozen) prefab.AddTag<Frozen>();
        if (hidden) prefab.AddTag<Hidden>();
        return prefab;
    }

    static PrefabEntity Plain2dPrefab(const float x = 1.f, const float y = 2.f)
    {
        PrefabEntity prefab;
        prefab.AddComponent<Position2d>(x, y);
        return prefab;
    }
};


TEST_F(TagsTest, TagRegistration_IsZeroSizedColumnless)
{
    const componentId_t id = ComponentRegistrator::GetComponentId<Frozen>();
    const auto& info = ComponentRegistrator::GetInfo(id);

    EXPECT_TRUE(info.isTag);
    EXPECT_EQ(info.componentSize, 0u);
}


TEST_F(TagsTest, PlainTagHead_YieldsEntity_AndFilters)
{
    std::set<Entity> frozen;
    frozen.insert(manager.Create<Entity>(FrozenPrefab(1.f, 1.f)));
    frozen.insert(manager.Create<Entity>(FrozenPrefab(2.f, 2.f)));
    manager.Create<Entity>(Plain2dPrefab());   // not frozen — must be skipped

    std::set<Entity> seen;
    for (auto [entity] : manager.view<Frozen>())     // plain tag head -> yields Entity
    {
        EXPECT_TRUE(manager.IsAlive(entity));
        seen.insert(entity);
    }

    EXPECT_EQ(seen, frozen);
}


TEST_F(TagsTest, PlainTagHead_WithComponent_YieldsEntityAndComponent)
{
    const Entity e = manager.Create<Entity>(FrozenPrefab(7.f, 8.f));
    manager.Create<Entity>(Plain2dPrefab());        // lacks Frozen

    size_t count = 0;
    for (auto [entity, pos] : manager.view<Frozen, Position2d>())
    {
        EXPECT_EQ(entity, e);
        EXPECT_FLOAT_EQ(pos.x, 7.f);
        EXPECT_EQ(&pos, &manager.GetComponent<Position2d>(entity));   // reference into storage
        ++count;
    }

    EXPECT_EQ(count, 1u);
}


TEST_F(TagsTest, TagInTail_IsFilterOnly_NotYielded)
{
    manager.Create<Entity>(FrozenPrefab(3.f, 3.f));
    manager.Create<Entity>(FrozenPrefab(4.f, 4.f));
    manager.Create<Entity>(Plain2dPrefab());        // no Frozen — filtered out

    // Frozen sits in the Tail: it filters but never appears in the tuple (arity 1).
    size_t count = 0;
    for (auto [pos] : manager.view<Position2d, Frozen>())
    {
        static_assert(std::is_same_v<decltype(pos), Position2d&>);
        EXPECT_GT(pos.x, 2.f);
        ++count;
    }

    EXPECT_EQ(count, 2u);
}


TEST_F(TagsTest, MultipleTailTags_AllFilterOnly)
{
    const Entity both = manager.Create<Entity>(FrozenPrefab(5.f, 5.f, true, true));
    manager.Create<Entity>(FrozenPrefab(6.f, 6.f, true, false));   // missing Hidden
    manager.Create<Entity>(Plain2dPrefab());                        // missing both

    std::set<Entity> seen;
    for (auto [entity, pos] : manager.view<Entity, Position2d, Frozen, Hidden>())
    {
        (void)pos;
        seen.insert(entity);
    }

    ASSERT_EQ(seen.size(), 1u);
    EXPECT_EQ(*seen.begin(), both);
}


TEST_F(TagsTest, WrapperTagHead_YieldsWrapper_AndFilters)
{
    PrefabEntity doorPrefab;
    doorPrefab.AddComponent<Position2d>(9.f, 9.f);
    doorPrefab.AddTag<Door>();

    const Entity door = manager.Create<Entity>(std::move(doorPrefab));
    manager.Create<Entity>(Plain2dPrefab());        // no Door tag — filtered out

    size_t count = 0;
    for (auto [wrapper] : manager.view<Door>())     // wrapper-tag head -> yields Door, filters on its bit
    {
        static_assert(std::is_same_v<decltype(wrapper), Door>);
        EXPECT_TRUE(wrapper.IsAlive());
        EXPECT_EQ(wrapper.getEntity(), door);
        EXPECT_FLOAT_EQ(wrapper.GetPosition().x, 9.f);
        ++count;
    }

    EXPECT_EQ(count, 1u);
}


TEST_F(TagsTest, WrapperTagHead_WithComponent_YieldsWrapperAndComponent)
{
    PrefabEntity doorPrefab;
    doorPrefab.AddComponent<Position2d>(2.f, 3.f);
    doorPrefab.AddTag<Door>();
    manager.Create<Entity>(std::move(doorPrefab));

    size_t count = 0;
    for (auto [door, pos] : manager.view<Door, Position2d>())
    {
        static_assert(std::is_same_v<decltype(door), Door>);
        EXPECT_EQ(&pos, &door.GetComponent<Position2d>());
        EXPECT_FLOAT_EQ(pos.x, 2.f);
        ++count;
    }

    EXPECT_EQ(count, 1u);
}


TEST_F(TagsTest, BaseWrapperHead_YieldsWrapper_WithoutExtraFilter)
{
    PrefabEntity withHealth;
    withHealth.AddComponent<Health>(80.f);
    withHealth.AddComponent<Position2d>(0.f, 0.f);
    const Entity e = manager.Create<Entity>(std::move(withHealth));

    // The base EntityWrapper is the one wrapper head carved out of IsTag: it selects the
    // handle type but adds NO filter bit, so it matches by the remaining components only.
    size_t count = 0;
    for (auto [wrapper, hp] : manager.view<EntityWrapper, Health>())
    {
        static_assert(std::is_same_v<decltype(wrapper), EntityWrapper>);
        EXPECT_EQ(wrapper.getEntity(), e);
        EXPECT_FLOAT_EQ(hp.value, 80.f);
        EXPECT_FLOAT_EQ(wrapper.GetComponent<Health>().value, 80.f);
        ++count;
    }

    EXPECT_EQ(count, 1u);
}


TEST_F(TagsTest, NamedWrapperHead_FiltersOnItsOwnTag)
{
    // A named wrapper (Player) is a tag: view<Player> filters on the Player bit, so the
    // entity must carry that tag to be matched.
    PrefabEntity tagged;
    tagged.AddComponent<Health>(70.f);
    tagged.AddTag<Player>();
    const Entity playerEntity = manager.Create<Entity>(std::move(tagged));

    PrefabEntity untagged;                       // has Health but is not a Player
    untagged.AddComponent<Health>(30.f);
    manager.Create<Entity>(std::move(untagged));

    size_t count = 0;
    for (auto [player, hp] : manager.view<Player, Health>())
    {
        static_assert(std::is_same_v<decltype(player), Player>);
        EXPECT_EQ(player.getEntity(), playerEntity);
        EXPECT_FLOAT_EQ(hp.value, 70.f);
        EXPECT_FLOAT_EQ(player.GetHealth(), 70.f);
        ++count;
    }

    EXPECT_EQ(count, 1u);
}


TEST_F(TagsTest, GetArchetype_ReflectsComponentsAndTags)
{
    const Entity e = manager.Create<Entity>(FrozenPrefab(1.f, 1.f));

    const Archetype& arch = manager.GetArchetype(e);
    EXPECT_TRUE(arch.test(ComponentRegistrator::GetComponentId<Position2d>()));
    EXPECT_TRUE(arch.test(ComponentRegistrator::GetComponentId<Frozen>()));
    EXPECT_FALSE(arch.test(ComponentRegistrator::GetComponentId<Hidden>()));
    EXPECT_FALSE(arch.test(ComponentRegistrator::GetComponentId<Position3d>()));
}


TEST_F(TagsTest, AddTag_MigratesArchetype_KeepsComponentData)
{
    const Entity e = manager.Create<Entity>(Plain2dPrefab(11.f, 22.f));
    ASSERT_FALSE(manager.GetArchetype(e).test(ComponentRegistrator::GetComponentId<Frozen>()));

    manager.AddTag<Frozen>(e);

    EXPECT_TRUE(manager.GetArchetype(e).test(ComponentRegistrator::GetComponentId<Frozen>()));
    // The tag carries no column, so the existing component data survives the migration.
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(e).x, 11.f);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(e).y, 22.f);

    size_t frozenCount = 0;
    for (auto [entity] : manager.view<Frozen>()) { (void)entity; ++frozenCount; }
    EXPECT_EQ(frozenCount, 1u);
}


TEST_F(TagsTest, AddTag_Idempotent_WhenAlreadyPresent)
{
    const Entity e = manager.Create<Entity>(FrozenPrefab(1.f, 1.f));
    const Archetype before = manager.GetArchetype(e);

    manager.AddTag<Frozen>(e);   // already present -> no-op

    EXPECT_EQ(manager.GetArchetype(e), before);
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(e).x, 1.f);
}


TEST_F(TagsTest, RemoveTag_ClearsBit_KeepsComponents)
{
    const Entity e = manager.Create<Entity>(FrozenPrefab(4.f, 5.f));
    ASSERT_TRUE(manager.GetArchetype(e).test(ComponentRegistrator::GetComponentId<Frozen>()));

    manager.RemoveTag<Frozen>(e);

    EXPECT_FALSE(manager.GetArchetype(e).test(ComponentRegistrator::GetComponentId<Frozen>()));
    EXPECT_FLOAT_EQ(manager.GetComponent<Position2d>(e).x, 4.f);

    size_t frozenCount = 0;
    for (auto [entity] : manager.view<Frozen>()) { (void)entity; ++frozenCount; }
    EXPECT_EQ(frozenCount, 0u);
}
