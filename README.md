<h1 align="center">R-ECS</h1>

<p align="center">
  <b>Replication · Entities · Components · Systems</b><br>
  A fast, header-only <b>archetype-based Entity-Component-System</b> framework for modern C++20.
</p>

<p align="center">
  <a href="https://github.com/semenshestakov/R-ECS/actions/workflows/ci.yml"><img alt="CI" src="https://github.com/semenshestakov/R-ECS/actions/workflows/ci.yml/badge.svg?branch=master"></a>
  <a href="https://github.com/semenshestakov/R-ECS/actions/workflows/docs.yml"><img alt="Docs" src="https://github.com/semenshestakov/R-ECS/actions/workflows/docs.yml/badge.svg?branch=master"></a>
  <a href="https://semenshestakov.github.io/R-ECS/"><img alt="Docs site" src="https://img.shields.io/badge/docs-online-success?logo=readthedocs&logoColor=white"></a>
  <a href="LICENSE.md"><img alt="License: MIT" src="https://img.shields.io/badge/license-MIT-blue"></a>
  <img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-00599C?logo=c%2B%2B&logoColor=white">
  <img alt="Header-only" src="https://img.shields.io/badge/header--only-yes-brightgreen">
  <a href="https://github.com/semenshestakov/R-ECS/stargazers"><img alt="Stars" src="https://img.shields.io/github/stars/semenshestakov/R-ECS?style=social"></a>
</p>

<p align="center">
  🌐 <a href="https://semenshestakov.github.io/R-ECS/"><b>Live docs site</b></a> &nbsp;·&nbsp;
  🚀 <a href="examples/README.md">Examples</a> &nbsp;·&nbsp;
  🇬🇧 <a href="docs/en/README.md">English docs</a> &nbsp;·&nbsp;
  🇷🇺 <a href="docs/ru/README.md">Русская документация</a>
</p>

---

## Contents

- [What is R-ECS?](#-what-is-r-ecs)
- [Why R-ECS?](#-why-r-ecs)
- [Quick start](#-quick-start)
- [Install](#-install)
- [Core concepts](#-core-concepts)
- [Examples](#-examples)
- [Benchmarks](#-benchmarks)
- [Documentation](#-documentation)
- [Roadmap](#-roadmap)
- [Contributing](#-contributing)
- [License](#-license)

## ⚡ What is R-ECS?

R-ECS is a high-performance **Entity-Component-System** framework written in
**C++20**, aimed at game development and real-time simulation. It uses an
**archetype-based** architecture: entities that share the same set of components
are stored together in packed chunks, giving excellent cache locality on
iteration while keeping the public API small and declarative.

```cpp
for (auto [pos, vel] : registry.Entities().view<Position2d, Velocity2d>())
{
    pos.x += vel.x;
    pos.y += vel.y;
}
```

## 🆚 Why R-ECS?

- **Archetype storage** — dense, packed chunks with swap-remove; views walk
  contiguous memory for cache-friendly iteration.
- **Declarative systems** — inherit `ISystem<T>`, tag with `ECS_REGISTRY`, and
  the framework auto-registers and schedules them. No manual wiring.
- **DAG scheduling** — declare ordering with `ECS_DEPENDENT_SYSTEMS`; systems
  (and their event handlers) run in deterministic topological order.
- **Type-safe events** — subscribe with one `ECS_EVENT` line; dispatch
  immediately (`OnEvent`) or defer to the frame boundary (`PushEvent`).
- **Deferred commands** — create / destroy / add / remove components safely
  between frames, even while iterating a view.
- **Prefabs, recipes & cooking** — assemble entities from reusable recipes and
  let systems enrich them via `PrefabEvt` / `CreatedEntityEvt`.
- **Shared context** — a type-indexed singleton store for resources and services.
- **Header-only & easy to drop in** — one `add_subdirectory` or `FetchContent`,
  no link step, no external runtime dependencies.
- **Benchmarked** — measured against [EnTT](https://github.com/skypjack/entt) and
  [flecs](https://github.com/SanderMertens/flecs) with Google Benchmark
  (see [Benchmarks](#-benchmarks)).

## 🚀 Quick start

```cpp
#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"

struct Position2d { float x, y; };
struct Velocity2d { float x, y; };

struct MoveSystem final : ecs::ISystem<MoveSystem>
{
    ECS_REGISTRY("game")

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
    ecs::Registry registry = ecs::Registry::Create("game");
    registry.Init();

    ecs::PrefabEntity prefab;
    prefab.AddComponent<Position2d>(0.f, 0.f);
    prefab.AddComponent<Velocity2d>(1.f, 2.f);

    for (int i = 0; i < 100; ++i)
        registry.Entities().Create(prefab);

    for (int frame = 0; frame < 60; ++frame)
        registry.Update();
}
```

## 📦 Install

R-ECS is a **header-only** CMake library requiring a **C++20** compiler.

**Option A — `FetchContent` (no checkout needed):**

```cmake
include(FetchContent)
FetchContent_Declare(
    R-ECS
    GIT_REPOSITORY https://github.com/semenshestakov/R-ECS.git
    GIT_TAG        master   # or pin a release tag
)
FetchContent_MakeAvailable(R-ECS)

target_link_libraries(my_game PRIVATE R-ECS)
```

**Option B — vendored as a subdirectory:**

```cmake
add_subdirectory(R-ECS)
target_link_libraries(my_game PRIVATE R-ECS)
```

**Option C — compile a single file directly** (since it's header-only):

```bash
c++ -std=c++20 -I include examples/Simple.cpp -o /tmp/sim && /tmp/sim
```

## 🧩 Core concepts

| Term | What it is |
|------|------------|
| **Registry** | The world. Owns entities, systems, events, context, and the command queue. Drive it with `Update()`. |
| **Entity** | A lightweight `{id, version}` handle. Holds no data itself. |
| **Component** | A plain struct of data attached to an entity. |
| **Archetype** | The set of component types an entity has; entities with the same archetype are stored together. |
| **System** | Logic that runs each frame (`ISystem<T>`), registered via `ECS_REGISTRY` and ordered via `ECS_DEPENDENT_SYSTEMS`. |
| **View** | A typed iterator (`view<A, B>()`) over all entities holding the requested components. |
| **Event** | A type-safe message; handlers subscribe with `ECS_EVENT` and run immediately or deferred. |
| **Command** | A deferred structural change (create/destroy/add/remove) flushed at the frame boundary. |
| **Recipe / Cooking** | A reusable blueprint (`Recipe`) cooked into an entity, with hooks to enrich it. |

## 🛠 Examples

Small, runnable programs in [`examples/`](examples/README.md), each mapped to a guide:

| Example | Shows |
|---------|-------|
| [`Simple.cpp`](examples/Simple.cpp) | Components, prefab, a movement system, the frame loop |
| [`SystemsWithDependencies.cpp`](examples/SystemsWithDependencies.cpp) | `ECS_DEPENDENT_SYSTEMS` and DAG ordering |
| [`Events.cpp`](examples/Events.cpp) | `ECS_EVENT`, `OnEvent` vs `PushEvent` |
| [`Commands.cpp`](examples/Commands.cpp) | Deferred create / add / remove / destroy |
| [`RecipesAndCooking.cpp`](examples/RecipesAndCooking.cpp) | `Recipe`, `CookCmd`, prefab/created events |

## 📊 Benchmarks

R-ECS ships a benchmark suite that pits it against **EnTT** and **flecs** on
create / view / delete workloads using Google Benchmark. The sources live in
[`benchmarks/entities/`](benchmarks/entities) so you can reproduce the numbers
on your own hardware and compiler:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE=ON
cmake --build build
./build/recs_bench
```

> Benchmark results are hardware-, compiler-, and flag-dependent — run them
> locally rather than trusting a single published figure.

## 📚 Documentation

The full docs — guides plus a generated API reference — live under
[`docs/`](docs/README.md) and are available in **English** and **Russian**, and
online at the [**live docs site**](https://semenshestakov.github.io/R-ECS/).

| | |
|---|---|
| 🇬🇧 English | [`docs/en/README.md`](docs/en/README.md) |
| 🇷🇺 Русский | [`docs/ru/README.md`](docs/ru/README.md) |

The API reference is generated from source docstrings with **Doxygen +
[moxygen2](https://github.com/matusnovak/moxygen2)**:

```bash
docs/scripts/generate-docs.sh        # builds docs/en/api and docs/ru/api
```

See [`docs/README.md`](docs/README.md) for prerequisites and how to add another
language.

## 🗺 Roadmap

| Milestone | Version | Tests |
|:----------|:-------:|:-----:|
| DynamicComponentsFactory *(deprecated)* | v0.1.0 ⚠️ | ⚠️ |
| ComponentsFactory *(deprecated)* | v0.2.0 ⚠️ | ⚠️ |
| Registry, auto-registration, iterators | v0.3.0 ✅ | ✅ |
| Event system | v0.4.0 ✅ | ✅ |
| EventSystem for ECS | v0.5.0 ✅ | ✅ |
| ECS context | v0.6.0 ✅ | ✅ |
| Directed Acyclic Graph, Systems Schedule | v0.7.0 ✅ | ✅ |
| Refactor entities · PrefabEntity, EntityWrapper, Archetype, EntitiesArchetypeStorage | v0.8.0 ✅ | ✅ |
| Event System: PushEvent / FlushEvents / priority | v0.9.0 ✅ | ✅ |
| Benchmarks vs EnTT & flecs (create / view / delete) | v0.10.0 ✅ | ✅ |
| Cooking: Cooker, Recipe (PrefabEntity hierarchy) | v0.11.0 ✅ | ✅ |
| Multithreading: ThreadPool, job queues, priority jobs, JobManager, safe parallel updates | v0.12.0 🚧 | 🚧 |
| Replication core: delta compression, RPC, pluggable network transport in `Registry` (virtual hooks — bring your own transport) | v0.13.0 📝 | 📝 |

<sub>✅ done · 🚧 in progress · 📝 planned · ⚠️ deprecated · ❌ not started</sub>

The **replication core** stays transport-agnostic: R-ECS provides the
delta-compression and RPC machinery, while the actual sockets are yours — plug a
network transport into `Registry` through virtual hooks and drive replication
over whatever protocol your project already uses.

## 🤝 Contributing

Contributions, bug reports, and feature ideas are welcome.

1. Read the guides under [`docs/en/`](docs/en/README.md) — they are the source of truth for current behavior.
2. Open an [issue](https://github.com/semenshestakov/R-ECS/issues) to discuss larger changes first.
3. Keep diffs small, follow the existing patterns, and update the matching guide when you change public APIs or macros.

If R-ECS is useful to you, **⭐ star the repo** — it genuinely helps others find it.

## 📄 License

Released under the **MIT License** — see [LICENSE.md](LICENSE.md).
