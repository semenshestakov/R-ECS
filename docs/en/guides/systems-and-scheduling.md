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

### Two kinds of dependency edge

Every edge in the schedule carries a kind (`ecs::SystemDep`):

- `Direct` — a **hard** dependency from `ECS_DEPENDENT_SYSTEMS`. It orders
  execution *and* propagates disabling (see below).
- `Data` — a **soft** dependency. It orders execution but never propagates
  disabling. Data edges come from declared component access and from
  `ECS_WEAK_DEPENDENT_SYSTEMS`.

A single edge can be both at once (the flags are OR-ed together).

### When the schedule is rebuilt

Adding a system does **not** recompute the schedule. `Add` only records the
system and marks the schedule dirty; the topological sort and data-edge
derivation run lazily on the next `Build()`, which the systems manager triggers
at the start of `Update()` (and `Subscribe()`). So registering systems is cheap,
and the dependency recomputation happens once at update time rather than on every
`Add`. A clean schedule makes `Build()` a no-op.

Because event-handler dispatch order is derived from the schedule (earlier stages
get higher priority), `Registry::Update` refreshes event ordering whenever the
schedule was recomputed. Existing handlers are **not** re-subscribed: their
priorities are updated in place via `SetEventsPriority` (which forwards to
`Event::setPriority`), so dispatch order tracks the new stages without churn.
Systems registered after the initial subscription are subscribed once on the next
refresh.

## Component access — `ECS_ACCESS`

Instead of (or in addition to) hard edges, a system can declare which components
it reads and writes. The scheduler turns that into ordering automatically:

```cpp
struct MovementSystem final : ecs::ISystem<MovementSystem>
{
    ECS_REGISTRY("game")
    ECS_ACCESS(ecs::Write<Position2d>, ecs::Read<Velocity2d>)

    void Update(ecs::Registry&, const ecs::UpdateState&) override { /* ... */ }
};
```

Accessor tags are `ecs::Read<C>` (read) and `ecs::Write<C>` (write). From the
declared access the scheduler derives `Data` edges so that:

- every **reader** of a component runs after every **writer** of it
  (writer-before-reader), and
- two **writers** of the same component are ordered deterministically (the
  smaller system hash runs first).

These derived edges only constrain order — they never cause a system to be
disabled when another is disabled.

## Weak dependencies — `ECS_WEAK_DEPENDENT_SYSTEMS`

Use a weak dependency when a system should run *after* another but must keep
running even if that other system is turned off:

```cpp
struct HudSystem final : ecs::ISystem<HudSystem>
{
    ECS_REGISTRY("game")
    ECS_WEAK_DEPENDENT_SYSTEMS(ScoreSystem)   // after ScoreSystem, but independent

    void Update(ecs::Registry&, const ecs::UpdateState&) override { /* ... */ }
};
```

This adds a `Data` edge: ordering is enforced, but disabling `ScoreSystem` leaves
`HudSystem` running.

## Enabling and disabling systems

Systems can be turned on and off at runtime through the systems manager:

```cpp
registry.Systems().Disable<PhysicsSystem>();
bool on = registry.Systems().IsEnabled<PhysicsSystem>();
registry.Systems().Enable<PhysicsSystem>();
```

A disabled system stops receiving `Update()` calls **and** its event handlers
are detached, so it no longer reacts to events either; re-enabling re-subscribes
them. The reconciliation happens at the start of the next `Update()`.
Disabling **cascades along `Direct` edges, transitively**: every system that
hard-depends on a disabled system — directly or through a chain — is skipped too,
and is unsubscribed from events along with it.
Systems linked only by component access or `ECS_WEAK_DEPENDENT_SYSTEMS` keep
running.

```
            disable ─┐
ResourceSystem       ▼
InputSystem ─▶ RenderSystem ─▶ PostRenderSystem   (both skipped: hard chain)
PhysicsSystem
HudSystem ⇢ RenderSystem                          (weak edge: keeps running)
```

`Enable` only clears the explicit disable; a system stays inactive while it still
hard-depends on something that is disabled. `IsEnabled` reflects the full cascade.

These calls are main-thread-only. To toggle a system from inside another system
running on a worker thread, defer it through the command queue with
[`DisableSystemCmd` / `EnableSystemCmd`](./commands.md#disablesystemcmd--enablesystemcmd),
which apply the change on the Registry's thread at the next flush.

## Accessing other systems and shared state

Inside `Update` (or anywhere with a `Registry&`):

```cpp
auto& physics = registry.Systems().Get<PhysicsSystem>();   // by type
auto* maybe   = registry.Systems().TryGet<AudioSystem>();  // nullptr if absent

// Shared, type-indexed singletons live in the context:
auto& clock = registry.ctx().getOrEmplace<GameClock>();
```

`registry.ctx()` returns an [`ecs::Context`](./jobs.md) — a main-thread-guarded
adapter over [`collections::Context`](../api/index_classes.md) — storing one
instance per type, ideal for resources and services shared across systems.
Mutating it (`getOrEmplace` / `emplace` / `remove`) is main-thread only; read it
with `get<T>()` from parallel systems.

## Manual registration (without names)

If you prefer not to use named registries, build a registry by hand:

```cpp
ecs::Registry registry;                 // empty
registry.Systems().Register<PhysicsSystem>();
registry.Systems().Register<RenderSystem>();
registry.Init();
```

Systems within one stage are dispatched through the job scheduler and may run
in parallel; see [Jobs & threading](./jobs.md#parallel-system-execution).

## See also

- [Jobs & threading](./jobs.md) — how stages are dispatched in parallel.
- [Events](./events.md) — system event handlers also follow the DAG order.
- [Getting started](./getting-started.md#the-frame-loop) — where `Update` sits in the frame.
