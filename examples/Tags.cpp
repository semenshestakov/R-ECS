// Tags & view head semantics.
//
// A Tag is a zero-sized marker that lives in an entity's archetype (it occupies a
// bit) but stores no per-entity data — no chunk column is ever allocated for it.
// Tags are therefore pure filters. This example shows the four ways a view can be
// led, and how tags behave as filters and as wrapper handles.

#include <iostream>

#include "ecs/components/ComponentRegistrator.hpp"
#include "ecs/entities/EntitiesManager.hpp"
#include "ecs/entities/EntityWrapper.hpp"
#include "ecs/entities/PrefabEntity.hpp"
#include "ecs/jobs/ThreadAffinity.hpp"


struct Position2d
{
    float x, y;
};

struct Health
{
    float value;
};

// Plain tags: filter-only markers.
struct Frozen final : ecs::Tag {};
struct Boss   final : ecs::Tag {};

// A wrapper-tag. EntityWrapper derives from ecs::Tag, so any named wrapper subclass is
// itself a tag: as a view head it yields the wrapper handle AND filters on its own bit,
// all while staying sizeof(EntityWrapper). (The base EntityWrapper is the one wrapper
// head that does not filter.)
struct Door final : ecs::EntityWrapper
{
    using EntityWrapper::EntityWrapper;
    [[nodiscard]] Position2d& GetPosition() { return GetComponent<Position2d>(); }
};


int main()
{
    ecs::MarkMainThread();

    ecs::EntitiesManager world;

    // Two frozen actors, one free actor, one boss, and a door.
    auto makeActor = [&](const float x, const bool frozen, const bool boss)
    {
        ecs::PrefabEntity p;
        p.AddComponent<Position2d>(x, 0.f);
        p.AddComponent<Health>(100.f);
        if (frozen) p.AddTag<Frozen>();
        if (boss)   p.AddTag<Boss>();
        return world.Create<ecs::Entity>(std::move(p));
    };

    makeActor(1.f, /*frozen*/ true,  /*boss*/ false);
    makeActor(2.f, /*frozen*/ true,  /*boss*/ true);
    const ecs::Entity free = makeActor(3.f, /*frozen*/ false, /*boss*/ false);

    {
        ecs::PrefabEntity doorPrefab;
        doorPrefab.AddComponent<Position2d>(9.f, 9.f);
        doorPrefab.AddTag<Door>();
        world.Create<ecs::Entity>(std::move(doorPrefab));
    }

    // 1) Plain tag as head -> yields the Entity, filters on the tag.
    std::cout << "Frozen entities (tag head yields Entity):\n";
    for (auto [entity] : world.view<Frozen>())
        std::cout << "  id=" << entity.id << '\n';

    // 2) Tag in the tail -> filter-only, never appears in the tuple.
    std::cout << "Frozen positions (tag filters, only Position2d yielded):\n";
    for (auto [pos] : world.view<Position2d, Frozen>())
        std::cout << "  x=" << pos.x << '\n';

    // Stack tags to narrow the query: frozen AND boss.
    std::cout << "Frozen bosses:\n";
    for (auto [entity, pos] : world.view<ecs::Entity, Position2d, Frozen, Boss>())
        std::cout << "  id=" << entity.id << " x=" << pos.x << '\n';

    // 3) Wrapper-tag as head -> yields the wrapper AND filters on its bit.
    std::cout << "Doors (wrapper-tag head yields Door):\n";
    for (auto [door] : world.view<Door>())
        std::cout << "  x=" << door.GetPosition().x << '\n';

    // Mutate tags at runtime; the archetype migration keeps component data intact.
    world.AddTag<Frozen>(free);
    std::cout << "After freezing id=" << free.id << ", frozen count = ";
    std::size_t frozen = 0;
    for (auto [e] : world.view<Frozen>()) { (void)e; ++frozen; }
    std::cout << frozen << '\n';

    // Inspect an entity's archetype directly.
    const ecs::Archetype& arch = world.GetArchetype(free);
    std::cout << "id=" << free.id << " has Frozen bit: "
              << std::boolalpha
              << arch.test(ecs::ComponentRegistrator::GetComponentId<Frozen>()) << '\n';

    // A view yields exactly one handle, chosen by Head — so the following would be a
    // compile error (a second handle in the tail is rejected by a static_assert):
    //
    //     world.view<ecs::Entity, Position2d, Door>();   // Door is a handle, not a component
    //     world.view<Frozen, ecs::Entity>();             // Entity may only be the head

    return 0;
}
