# Getting started

[🇷🇺 Русский](../../ru/guides/getting-started.md) · [⬆ English docs](../README.md)

## Add R-ECS to your build

R-ECS is a **header-only** library exposed through CMake. Drop the repository
into your project (submodule, `FetchContent`, or vendored folder) and link it:

```cmake
add_subdirectory(R-ECS)

target_link_libraries(my_game PRIVATE R-ECS)
```

Tests and benchmarks build **only** when R-ECS is the top-level project, so
embedding it as a subdirectory keeps your build lean. The library requires a
C++20 compiler.

## The mental model

R-ECS has four moving parts, all owned by a single `ecs::Registry`:

- **Components** — plain structs holding data (no logic).
- **Entities** — lightweight `{id, version}` handles that own a set of
  components (their *archetype*).
- **Systems** — behaviour. Each frame the registry calls `Update` on every
  system in dependency order.
- **Events & Commands** — type-safe queues for cross-system messages and for
  deferred structural changes.

## Your first program

```cpp
#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/entities/PrefabEntity.hpp"

struct Position2d { float x, y; };
struct Velocity2d { float x, y; };

// A system: register it under a name and implement Update().
struct MoveSystem final : ecs::ISystem<MoveSystem>
{
    ECS_REGISTRY("game")          // associates this system with registry "game"

    void Update(ecs::Registry& registry, const ecs::UpdateState&) override
    {
        for (auto [pos, vel] : registry.Entities().view<Position2d, Velocity2d>())
        {
            pos.x += vel.x;
            pos.y += vel.y;
        }
    }
};

int main()
{
    // Build a registry pre-populated with every system registered as "game".
    ecs::Registry registry = ecs::Registry::Create("game");
    registry.Init();

    // Assemble an entity from a prefab.
    ecs::PrefabEntity prefab;
    prefab.AddComponent<Position2d>(0.f, 0.f);
    prefab.AddComponent<Velocity2d>(1.f, 2.f);

    for (int i = 0; i < 100; ++i)
        registry.Entities().Create(prefab);

    // Main loop.
    for (int frame = 0; frame < 60; ++frame)
        registry.Update();
}
```

> **Two ways to build a `Registry`.** `Registry::Create("game")` looks up every
> system tagged with `ECS_REGISTRY("game")` and wires them in automatically.
> You can also default-construct `ecs::Registry registry;` and register systems
> by hand through `registry.Systems()`.

## The frame loop

`registry.Update()` runs one frame, in this exact order:

1. **Flush pending commands** queued *before* the frame.
2. **Update every system** in topological (dependency) order.
3. **Flush queued events** (`PushEvent`) to their subscribers.
4. **Flush commands** queued *during* the frame.

```cpp
while (running)
{
    pollInput();
    registry.Update();   // systems + events + commands
    render();
}
```

`registry.Init()` must be called once before the first `Update()`: it
initializes systems and subscribes their event handlers.

## Where to go next

- [Components & prefabs](./components-and-prefabs.md) — define data and build entities.
- [Entities & views](./entities-and-views.md) — query and mutate entities.
- [Systems & scheduling](./systems-and-scheduling.md) — order systems with a DAG.
- [Events](./events.md) and [Commands](./commands.md) — communicate and defer work.
