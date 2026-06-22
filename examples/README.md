# Examples

Small, self-contained programs that each demonstrate one part of R-ECS. They map
directly onto the guides in [`docs/`](../docs/README.md).

| File                                                           | Demonstrates | Guide |
|----------------------------------------------------------------|--------------|-------|
| [`Simple.cpp`](./Simple.cpp)                                   | Components, prefab, a movement system, the frame loop | [Getting started](../docs/en/guides/getting-started.md) |
| [`SystemsWithDependencies.cpp`](./SystemsWithDependencies.cpp) | `ECS_DEPENDENT_SYSTEMS` and DAG ordering | [Systems & scheduling](../docs/en/guides/systems-and-scheduling.md) |
| [`Events.cpp`](./Events.cpp)                                   | `ECS_EVENT`, `OnEvent` vs `PushEvent` | [Events](../docs/en/guides/events.md) |
| [`Commands.cpp`](./Commands.cpp)                               | `CreateEntityCmd` / `AddComponentsCmd` / `RemoveComponentsCmd` / `DeleteEntityCmd` | [Commands](../docs/en/guides/commands.md) |
| [`RecipesAndCooking.cpp`](./RecipesAndCooking.cpp)             | `Recipe`, `CookCmd`, `PrefabEvt` / `CreatedEntityEvt` | [Recipes & cooking](../docs/en/guides/recipes-and-cooking.md) |

## Building an example

The examples are not part of the default CMake target. Because R-ECS is
header-only you can compile a single file directly:

```bash
c++ -std=c++20 -I include examples/Events.cpp -o /tmp/events && /tmp/events
```

Or add an executable to your own CMake project:

```cmake
add_executable(ecs_events examples/Events.cpp)
target_link_libraries(ecs_events PRIVATE R-ECS)
```
