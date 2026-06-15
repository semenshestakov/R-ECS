# R-ECS — English Documentation

[🇷🇺 Русский](../ru/README.md) · [⬆ Docs home](../README.md) · [📦 Repository](../../README.md)

R-ECS is an **archetype-based Entity-Component-System** framework for modern
C++ (C++20). Entities are stored in packed chunks grouped by their component
set (archetype), giving cache-friendly iteration; systems are scheduled with a
dependency DAG; events and structural changes flow through type-safe queues.

## 🚀 Start here

| Guide | What it covers |
|-------|----------------|
| [Getting started](./guides/getting-started.md) | Build, the 30-line first program, the frame loop |
| [Components & prefabs](./guides/components-and-prefabs.md) | Defining components, `PrefabEntity`, `AddComponent` |
| [Entities & views](./guides/entities-and-views.md) | `Create`, `GetComponent`, `view<...>`, `EntityWrapper` |
| [Systems & scheduling](./guides/systems-and-scheduling.md) | `ISystem`, `ECS_REGISTRY`, `ECS_DEPENDENT_SYSTEMS`, the DAG |
| [Events](./guides/events.md) | `ECS_EVENT`, `OnEvent` vs `PushEvent`, priority |
| [Commands](./guides/commands.md) | Deferred `CreateEntityCmd` / `DeleteEntityCmd` / `AddComponentsCmd` |
| [Recipes & cooking](./guides/recipes-and-cooking.md) | `Recipe`, `CookCmd`, `PrefabEvt` / `CreatedEntityEvt` |
| [Jobs & threading](./guides/jobs.md) | `IJobScheduler`, `SetScheduler`, pluggable parallel backends |

## 📚 API reference

Generated from the source docstrings (run
[`docs/scripts/generate-docs.sh`](../scripts/generate-docs.sh) first):

- [Classes](./api/index_classes.md)
- [Namespaces](./api/index_namespaces.md)
- [Files](./api/index_files.md)

## 🗺 Library map — by `include/` directory

### `include/ecs` — core façade

| Header | Role | Guide |
|--------|------|-------|
| `ecs/Registry.hpp` | Central hub: owns entities, events, systems, context, command queue | [Getting started](./guides/getting-started.md) |
| `ecs/ISystem.hpp` | CRTP base for systems + the `ECS_*` macros | [Systems & scheduling](./guides/systems-and-scheduling.md) |

### `include/ecs/entities` — entities & storage

| Header | Role | Guide |
|--------|------|-------|
| `entities/EntitiesManager.hpp` | Entity lifecycle, `Create` / `Destroy` / `view` / `AddComponents` | [Entities & views](./guides/entities-and-views.md) |
| `entities/PrefabEntity.hpp` | Builder that assembles component data before creation | [Components & prefabs](./guides/components-and-prefabs.md) |
| `entities/EntityWrapper.hpp` | Safe handle (`IsAlive`, `GetComponent`, `SelfDestroy`) | [Entities & views](./guides/entities-and-views.md) |
| `entities/Archetype.hpp` | Component-set fingerprint used to group entities | [Entities & views](./guides/entities-and-views.md) |
| `entities/ArchetypedChunks.hpp` | Packed-chunk dense storage + view iterators | — |
| `entities/EntitiesArchetypeStorage.hpp` | Map of archetype → chunk storage | — |
| `entities/Entity.hpp` | The `{id, version}` handle | — |

### `include/ecs/components` — component registration

| Header | Role | Guide |
|--------|------|-------|
| `components/ComponentRegistrator.hpp` | Assigns stable component IDs, stores ctor/dtor/move | [Components & prefabs](./guides/components-and-prefabs.md) |
| `components/ComponentFreeList.hpp` | Per-type slot pool reused by prefabs | — |
| `components/Utils.hpp` | `IsComponent`, `componentId_t`, limits | [Components & prefabs](./guides/components-and-prefabs.md) |

### `include/ecs/systems` — scheduling & dispatch

| Header | Role | Guide |
|--------|------|-------|
| `systems/SystemsManager.hpp` | Owns system instances, runs `Init` / `Update` / `Subscribe` | [Systems & scheduling](./guides/systems-and-scheduling.md) |
| `systems/SystemsSchedule.hpp` | Topological order from the dependency DAG | [Systems & scheduling](./guides/systems-and-scheduling.md) |
| `systems/EventSystem.hpp` | ECS event bridge: `OnEvent` / `PushEvent` / `FlushEvents` | [Events](./guides/events.md) |
| `systems/SystemRegistrator.hpp` | Static auto-registration of system types | [Systems & scheduling](./guides/systems-and-scheduling.md) |
| `systems/IBaseSystem.hpp` | Non-templated system interface | — |

### `include/ecs/jobs` — threading port

| Header | Role | Guide |
|--------|------|-------|
| `jobs/IJobScheduler.hpp` | Abstract threading backend (`ParallelFor`, `Run`, `Wait`) | [Jobs & threading](./guides/jobs.md) |
| `jobs/SerialJobScheduler.hpp` | Default inline backend; core works with no threads | [Jobs & threading](./guides/jobs.md) |
| `jobs/JobHandle.hpp` | Opaque value handle to in-flight work | [Jobs & threading](./guides/jobs.md) |
| `jobs/FunctionRef.hpp` | Non-owning, allocation-free callable ref for hot loop bodies | [Jobs & threading](./guides/jobs.md) |

### `include/ecs/registry` — registry-level features

| Header | Role | Guide |
|--------|------|-------|
| `registry/Commands.hpp` | Deferred command types (`CreateEntityCmd`, `AddComponentsCmd`, `CookCmd`, …) | [Commands](./guides/commands.md) |
| `registry/Events.hpp` | `PrefabEvt` / `CreatedEntityEvt` payloads | [Recipes & cooking](./guides/recipes-and-cooking.md) |
| `registry/CommandQueue.hpp` | Token-gated per-frame command queue | [Commands](./guides/commands.md) |
| `registry/RegistryRegistrator.hpp` | Maps a registry name to its systems (`Registry::Create`) | [Systems & scheduling](./guides/systems-and-scheduling.md) |

### `include/event` — generic event system

| Header | Role | Guide |
|--------|------|-------|
| `event/EventSystem.hpp` | Generic keyed dispatcher with priority | [Events](./guides/events.md) |
| `event/Listener.hpp` · `event/AbstractListener.hpp` | Typed listeners | [Events](./guides/events.md) |
| `event/Event.hpp` · `event/AbstractEvent.hpp` | Event payload base | [Events](./guides/events.md) |

### `include/collections` — reusable containers

| Header | Role | Guide |
|--------|------|-------|
| `collections/Context.hpp` | Type-indexed singleton store (`ctx()`) | [Getting started](./guides/getting-started.md) |
| `collections/CommandQueue.hpp` | Generic deferred-callable queue | [Commands](./guides/commands.md) |
| `collections/DirectedAcyclicGraph.hpp` | DAG behind system scheduling | [Systems & scheduling](./guides/systems-and-scheduling.md) |
| `collections/BitSet.hpp` | Fixed-width bitset used by archetypes | — |

### `include/reg` & `include/common_recs`

| Header | Role |
|--------|------|
| `reg/Registrator.hpp` | RAII name/type registration with `DEFAULT` / `UNIQUE` strategies |
| `common_recs/utils/BaseError.hpp` | Named error base types |
| `common_recs/utils/ClassUtils.hpp` | Type-name / hashing helpers |

---

<sub>The API reference under `api/` is generated — see
[building the docs](../README.md#building-the-api-reference).</sub>
