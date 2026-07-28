# Examples

Small, self-contained programs that each demonstrate one part of R-ECS. They map
directly onto the guides in [`docs/`](../docs/README.md).

| File                                                           | Demonstrates | Guide |
|----------------------------------------------------------------|--------------|-------|
| [`Simple.cpp`](./Simple.cpp)                                   | Components, prefab, a movement system, the frame loop | [Getting started](../docs/en/guides/getting-started.md) |
| [`Tags.cpp`](./Tags.cpp)                                       | `ecs::Tag`, `AddTag` / `RemoveTag`, `view` head semantics, filter-only tags, wrapper-tags, `GetArchetype` | [Entities & views](../docs/en/guides/entities-and-views.md) |
| [`SystemsWithDependencies.cpp`](./SystemsWithDependencies.cpp) | `ECS_DEPENDENT_SYSTEMS` and DAG ordering | [Systems & scheduling](../docs/en/guides/systems-and-scheduling.md) |
| [`Events.cpp`](./Events.cpp)                                   | `ECS_EVENT`, `OnEvent` vs `PushEvent` | [Events](../docs/en/guides/events.md) |
| [`Commands.cpp`](./Commands.cpp)                               | `CreateEntityCmd` / `AddComponentsCmd` / `RemoveComponentsCmd` / `DeleteEntityCmd` | [Commands](../docs/en/guides/commands.md) |
| [`RecipesAndCooking.cpp`](./RecipesAndCooking.cpp)             | `Recipe`, `CookCmd`, `PrefabEvt` / `CreatedEntityEvt` | [Recipes & cooking](../docs/en/guides/recipes-and-cooking.md) |
| [`Jobs.cpp`](./Jobs.cpp)                                       | `TbbJobScheduler`, `UpdateState::Run` sync point, `ParallelFor`, manual `Run` / `Wait` | [Jobs & threading](../docs/en/guides/jobs.md) |
| [`Services.cpp`](./Services.cpp)                               | `TbbJobScheduler`, `SpawnService` / `StopService`, the per-frame `PumpServices` tick | [Jobs & threading](../docs/en/guides/jobs.md) |

## Building an example

The examples are gated behind the `EXAMPLES_ENABLE` CMake option. Each file is
built as `example_<Name>`. The core examples run on the default
`SerialJobScheduler`, so they need nothing extra:

```bash
cmake -B build -DEXAMPLES_ENABLE=ON
cmake --build build --target example_Simple
./build/example_Simple
```

`Jobs.cpp` and `Services.cpp` install `TbbJobScheduler`, so they additionally
need the oneTBB backend — enable it with `-DRECS_BACKEND_ONETBB=ON` (without it
those two targets are skipped):

```bash
cmake -B build -DEXAMPLES_ENABLE=ON -DRECS_BACKEND_ONETBB=ON
cmake --build build --target example_Jobs example_Services
./build/example_Jobs
```

R-ECS is not header-only — the core lives in `src/recs/*.cpp`, the TBB backend in
`src/recs-tbb/TbbJobScheduler.cpp` — and the codebase needs a C++20 toolchain
(GCC 12+/Clang 16+). The CMake build above wires the include paths
(`include/recs`, `include/recs-tbb`) and TBB linkage for you.
