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
#include <cstddef>
#include <iostream>

#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/entities/PrefabEntity.hpp"

struct Vel2d      { float x, y; };
struct Position2d { float x, y; };

// First system: integrate velocity and apply drag.
struct MoveSystem final : ecs::ISystem<MoveSystem>
{
    ECS_REGISTRY("MyName")

    void Update(ecs::Registry& registry, const ecs::UpdateState&) override
    {
        for (auto [pos, vel] : registry.Entities().view<Position2d, Vel2d>())
        {
            pos.x += vel.x;
            pos.y += vel.y;

            vel.x *= 0.98f;
            vel.y *= 0.98f;
        }
    }
};

// Second system: log positions — runs strictly after MoveSystem.
struct LogMoveSystem final : ecs::ISystem<LogMoveSystem>
{
    ECS_DEPENDENT_SYSTEMS(MoveSystem)
    ECS_REGISTRY("MyName")

    void Update(ecs::Registry& registry, const ecs::UpdateState&) override
    {
        std::cout << "[Logger] Move system started\n";
        std::size_t i = 0;
        for (auto [pos, vel] : registry.Entities().view<Position2d, Vel2d>())
            std::cout << "[" << ++i << "] \t" << pos.x << ", " << pos.y << "\n";
    }
};

int main()
{
    // Build a registry pre-populated with every system registered as "MyName".
    auto registry = ecs::Registry::Create("MyName");
    registry.Init();

    // Assemble entities from a prefab, then override each position individually.
    ecs::PrefabEntity prefab;
    prefab.AddComponent<Position2d>(0.f, 0.f);
    prefab.AddComponent<Vel2d>(100.f, 100.f);

    for (std::size_t i = 0; i < 100; ++i)
    {
        ecs::EntityWrapper entity = registry.Entities().Create(prefab);
        entity.GetComponent<Position2d>().x =  static_cast<float>(i);
        entity.GetComponent<Position2d>().y = -static_cast<float>(i);
    }

    // Main loop.
    for (std::size_t i = 0; i < 100; ++i)
    {
        std::cout << "Frame:" << i << "\n";
        registry.Update();
    }
}
```

> **Two ways to build a `Registry`.** `Registry::Create("MyName")` looks up every
> system tagged with `ECS_REGISTRY("MyName")` and wires them in automatically.
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
