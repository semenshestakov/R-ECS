// ───────────────────────────────────────────────────────────────────────
//  R-ECS example — deferred commands
//
//  Creating, mutating and destroying entities safely between frames with
//  CreateEntityCmd / AddComponentsCmd / RemoveComponentsCmd / DeleteEntityCmd.
//
//  See: docs/en/guides/commands.md
// ───────────────────────────────────────────────────────────────────────
#include <iostream>

#include "ecs/Registry.hpp"
#include "ecs/registry/Commands.hpp"

struct Position2d { float x{}, y{}; };
struct Health     { float value = 100.f; };
struct Damage     { float value = 10.f; };
struct Speed      { float value = 1.f; };

int main()
{
    ecs::Registry registry;   // no systems needed for this example
    auto& world = registry.Entities();

    // 1. Deferred create — capture the new entity in the callback.
    ecs::Entity spawned;
    ecs::PrefabEntity prefab;
    prefab.AddComponent<Position2d>(3.f, 4.f);

    registry.Commands().Push(ecs::CreateEntityCmd{
        std::move(prefab),
        [&](ecs::Entity e) { spawned = e; }
    });

    std::cout << "pending commands: " << registry.Commands().size() << "\n";
    registry.Update();                       // create flushes here
    std::cout << "alive after flush: " << world.IsAlive(spawned) << "\n";

    // 2. Deferred add — several components at once (types deduced).
    registry.Commands().Push(ecs::AddComponentsCmd{
        spawned, Health{50.f}, Damage{15.f}, Speed{3.f}
    });
    registry.Update();
    std::cout << "health = " << world.GetComponent<Health>(spawned).value << "\n";

    // 3. Deferred remove — list the component types as template params.
    registry.Commands().Push(ecs::RemoveComponentsCmd<Damage>{ spawned });
    registry.Update();
    std::cout << "has Damage: "
              << (world.TryGetComponent<Damage>(spawned) != nullptr) << "\n";

    // 4. Deferred destroy — callback runs while still alive.
    registry.Commands().Push(ecs::DeleteEntityCmd{
        spawned,
        [](ecs::Entity e) { std::cout << "destroying entity " << e.id << "\n"; }
    });
    registry.Update();
    std::cout << "entities left: " << world.size() << "\n";
}
