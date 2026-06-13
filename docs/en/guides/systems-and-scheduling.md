# Systems & scheduling

[🇷🇺 Русский](../../ru/guides/systems-and-scheduling.md) · [⬆ English docs](../README.md)

## Defining a system

A system inherits `ecs::ISystem<Derived>` (CRTP) and overrides `Update`:

```cpp
struct PhysicsSystem final : ecs::ISystem<PhysicsSystem>
{
    ECS_REGISTRY("game")

    void Update(ecs::Registry& registry, const ecs::UpdateState& state) override
    {
        for (auto [pos, vel] : registry.Entities().view<Position2d, Velocity2d>())
        {
            pos.x += vel.x;
            pos.y += vel.y;
        }
    }
};
```

Inheriting `ISystem<PhysicsSystem>` does two things automatically:

- registers the system type with `ecs::SystemRegistrator` during static
  initialization (no manual call needed), and
- gives it the `ECS_EVENT` / `ECS_DEPENDENT_SYSTEMS` machinery.

## ECS_REGISTRY — naming a system

`ECS_REGISTRY("name", ...)` declares the registry name(s) a system belongs to.
Every system sharing a name is wired together when you call
`Registry::Create("name")`:

```cpp
ecs::Registry registry = ecs::Registry::Create("game");
registry.Init();
```

A system can belong to several registries: `ECS_REGISTRY("game", "editor")`.

## Declaring dependencies — the DAG

Systems run in **topological order** derived from a dependency graph. Declare
what a system must run *after* with `ECS_DEPENDENT_SYSTEMS`:

```cpp
struct ResourceSystem final : ecs::ISystem<ResourceSystem> { ECS_REGISTRY("game") /* ... */ };
struct InputSystem    final : ecs::ISystem<InputSystem>    { ECS_REGISTRY("game") /* ... */ };
struct PhysicsSystem  final : ecs::ISystem<PhysicsSystem>  { ECS_REGISTRY("game") /* ... */ };

struct RenderSystem final : ecs::ISystem<RenderSystem>
{
    ECS_REGISTRY("game")
    ECS_DEPENDENT_SYSTEMS(ResourceSystem, InputSystem, PhysicsSystem)

    void Update(ecs::Registry&, const ecs::UpdateState&) override { /* draw */ }
};

struct PostRenderSystem final : ecs::ISystem<PostRenderSystem>
{
    ECS_REGISTRY("game")
    ECS_DEPENDENT_SYSTEMS(RenderSystem)   // runs strictly after RenderSystem

    void Update(ecs::Registry&, const ecs::UpdateState&) override { /* present */ }
};
```

Given the above, the schedule guarantees:

```
ResourceSystem ┐
InputSystem    ├─▶ RenderSystem ─▶ PostRenderSystem
PhysicsSystem  ┘
```

`ResourceSystem`, `InputSystem` and `PhysicsSystem` have no ordering constraints
between themselves; only the declared edges are enforced. The graph is a
[`collections::DirectedAcyclicGraph`](../api/index_classes.md) and a cycle is a
setup error.

## Accessing other systems and shared state

Inside `Update` (or anywhere with a `Registry&`):

```cpp
auto& physics = registry.Systems().Get<PhysicsSystem>();   // by type
auto* maybe   = registry.Systems().TryGet<AudioSystem>();  // nullptr if absent

// Shared, type-indexed singletons live in the context:
auto& clock = registry.ctx().getOrEmplace<GameClock>();
```

The [`collections::Context`](../api/index_classes.md) (`registry.ctx()`) stores
one instance per type — ideal for resources and services shared across systems.

## Manual registration (without names)

If you prefer not to use named registries, build a registry by hand:

```cpp
ecs::Registry registry;                 // empty
registry.Systems().Register<PhysicsSystem>();
registry.Systems().Register<RenderSystem>();
registry.Init();
```

## See also

- [Events](./events.md) — system event handlers also follow the DAG order.
- [Getting started](./getting-started.md#the-frame-loop) — where `Update` sits in the frame.
