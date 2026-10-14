# Entities & views

[🇷🇺 Русский](../../ru/guides/entities-and-views.md) · [⬆ English docs](../README.md)

Everything about entities goes through `registry.Entities()`, which returns the
[`ecs::EntitiesManager`](../api/index_classes.md).

## Creating and destroying

```cpp
auto& world = registry.Entities();

ecs::EntityWrapper e = world.Create(prefab);  // build from a prefab
bool alive = e.IsAlive();                      // true

e.SelfDestroy();                               // destroy via the wrapper
// or: world.Destroy(e.getEntity());
```

An `ecs::Entity` is a tiny `{id, version}` handle. The version is bumped when an
id is recycled, so a stale handle to a destroyed entity reports `IsAlive() ==
false` instead of silently aliasing a new entity.

## EntityWrapper — the safe handle

`Create` returns an [`ecs::EntityWrapper`](../api/index_classes.md): an entity
plus a reference to its manager, so you can act on it directly.

```cpp
e.GetComponent<Position2d>().x = 5.f;          // asserts the component exists

if (auto* hp = e.TryGetComponent<Health>())    // nullptr if absent / dead
    hp->value -= 10.f;

ecs::entityId_t id = e.getId();
```

`GetComponent<T>()` asserts the entity is alive and owns `T`; `TryGetComponent<T>()`
returns a pointer (or `nullptr`) and never asserts.

## Iterating with views

`view<Components...>()` yields only the entities that own **all** of the listed
components. Each iteration step destructures into references you can read and
write in place.

```cpp
for (auto [pos, vel] : world.view<Position2d, Velocity2d>())
{
    pos.x += vel.x;        // references into storage — writes are persisted
    pos.y += vel.y;
}
```

Because entities are grouped by archetype in packed chunks, a view walks
contiguous memory — the cache-friendly core of the framework. A single-component
view works the same way:

```cpp
for (auto [hp] : world.view<Health>())
    if (hp.value <= 0.f) { /* mark for removal */ }
```

### Getting the entity handle

Only the **first** argument (`Head`) chooses the handle yielded alongside the
components; every later argument is a component (or a filter-only tag, see below).
Lead the list with `Entity` to also receive the owning entity handle as the first
element of each step. `Entity` is not a component — it is stripped from the archetype
filter, so the view still matches on the remaining components only:

```cpp
for (auto [entity, pos, vel] : world.view<Entity, Position2d, Velocity2d>())
{
    pos.x += vel.x;
    if (pos.x > bound) world.Destroy(entity);   // handle in hand, no extra lookup
}
```

`view<Entity>()` on its own walks every alive entity and yields just its handle.

A view yields **exactly one** handle, chosen by the head. Putting a second handle
after the head — another `Entity`, or any `EntityWrapper` type — is a **compile
error**:

```cpp
world.view<Entity, Position2d, Player>();   // error: Player is a handle, not a component
world.view<Frozen, Entity>();               // error: Entity may only be the head
```

You can also lead with a wrapper type to receive that wrapper instead of a raw
`Entity` (see [Typed wrappers](#typed-wrappers)). `EntityWrapper` derives from
`ecs::Tag`, so a **named** wrapper subclass is itself a tag and filters on its own
bit — `view<Player, Health>()` yields a `Player` only for entities that carry the
`Player` tag *and* own `Health`. The base `EntityWrapper` is the single exception: it
is carved out of the tag concept, so `view<EntityWrapper, Health>()` adds no filter
and yields a wrapper for every entity that owns `Health`.

### Optional components

Write a component as a pointer — `Component*` — to make it optional. Optional
components do **not** take part in the archetype filter: the view still selects
entities by its required (non-pointer) components, and for each match the optional
argument yields a pointer to the component when the entity owns it, or `nullptr`
otherwise. It is the iteration-time equivalent of `TryGetComponent`:

```cpp
for (auto [pos, vel] : world.view<Position2d, Velocity2d*>())
{
    pos.x += 1.f;                    // Position2d is required — a reference
    if (vel) pos.x += vel->x;        // Velocity2d is optional — pointer or nullptr
}
```

The markers compose with `Entity` and with each other:

```cpp
for (auto [entity, hp, shield] : world.view<Entity, Health, Shield*>())
    ...
```

If every listed component is optional, the required filter is empty and the view
walks every alive entity, reporting each optional component as present or null.

### Reading several components at once

`GetComponents<...>()` applies the very same `Component` / `Component*` rules to a
single entity by handle, returning a tuple instead of iterating. A required
`Component` comes back as a reference, an optional `Component*` as a pointer (null
when absent) — the bulk equivalent of `GetComponent` / `TryGetComponent`:

```cpp
auto [pos, vel] = world.GetComponents<Position2d, Velocity2d*>(entity);
pos.x += 1.f;
if (vel) pos.x += vel->x;
```

## Tags

A **tag** is a zero-sized marker that derives from `ecs::Tag`. It takes part in an
entity's archetype — it occupies a bit — but carries **no per-entity data**, so no
chunk column is ever allocated for it. Tags are pure filters.

```cpp
struct Frozen final : ecs::Tag {};
struct Boss   final : ecs::Tag {};
```

Attach tags when building a prefab with `AddTag<T>()`, or to a live entity with
`AddTag<Tags...>()` / `RemoveTag<Tags...>()` on the manager. Like `AddComponents`,
these migrate the entity to the resulting archetype but construct no data; they are
no-ops on a dead entity, and adding a tag already present does nothing:

```cpp
prefab.AddTag<Frozen>();               // on a prefab

world.AddTag<Frozen, Boss>(entity);    // on a live entity
world.RemoveTag<Frozen>(entity);
```

Check whether a live entity carries a tag with `HasTag<T>()` — the tag counterpart
of `TryGetComponent`: since a tag has no data there is nothing to return but a bool.
It is available on the manager and, for convenience, on `EntityWrapper`; both return
`false` (never assert) for a dead entity:

```cpp
bool frozen = world.HasTag<Frozen>(entity);   // on the manager
bool frozen2 = wrapper.HasTag<Frozen>();      // on an EntityWrapper handle
```

Each has a low-level, non-template counterpart taking a runtime `componentId_t` instead
of a compile-time tag type — the same split as `GetComponent<T>` / `GetComponentData(componentId)`:

```cpp
bool frozen3 = world.HasTagById(entity, ComponentRegistrator::GetComponentId<Frozen>());
bool frozen4 = wrapper.HasTagById(ComponentRegistrator::GetComponentId<Frozen>());
```

In a `view`, tags are **filter-only** wherever they appear — they contribute an
archetype bit but are never yielded and never allocate a column. A plain tag used as
the head still yields the `Entity`:

```cpp
for (auto [entity] : world.view<Frozen>())            // tag head -> yields Entity, filters on Frozen
    ...

for (auto [pos] : world.view<Position2d, Frozen>())   // Frozen filters; only Position2d is yielded
    ...

// Stack tags to narrow the query:
for (auto [entity, pos] : world.view<Entity, Position2d, Frozen, Boss>())
    ...                                               // frozen AND boss
```

### Wrapper-tags

`EntityWrapper` derives from `ecs::Tag`, so **every named wrapper subclass is a
tag** — no extra base is needed. Because `ecs::Tag` is empty it is folded away by
empty-base optimization, so a wrapper stays `sizeof(EntityWrapper)` and still
satisfies `EntityWrapperLike`. As a view head a named wrapper does **both** jobs: it
yields the wrapper *and* filters on its own bit. Creating an entity through
`Create<Door>(...)` stamps that bit automatically; you can also attach it like any
tag with `AddTag<Door>()`.

```cpp
struct Door final : ecs::EntityWrapper           // a named wrapper is already a tag
{
    using EntityWrapper::EntityWrapper;
    Position2d& GetPosition() { return GetComponent<Position2d>(); }
};

Door door = world.Create<Door>(std::move(prefab)); // Create<Door> sets the Door bit for you

for (auto [d] : world.view<Door>())              // yields Door, filters on the Door bit
    d.GetPosition().x += 1.f;
```

Contrast the handle heads: the base **`EntityWrapper`** selects a wrapper handle but
adds no filter; a **plain tag** (`Frozen`) yields the `Entity` and filters; a **named
wrapper** (`Door`, `Player`) yields that wrapper and filters on its own bit.

> A named wrapper filters on its own tag, so an entity is matched by `view<Door>()`
> only once it carries the `Door` bit. `Create<Door>(...)` sets that bit as part of
> creation — the wrapper's tag is stamped onto the archetype without touching the
> source prefab (a prefab reused for a differently-typed entity stays untagged). An
> entity built as `Create<Entity>(...)` still needs an explicit `AddTag<Door>()` to
> join the view.

### Inspecting an entity's archetype

`GetArchetype(entity)` returns a const reference to the entity's archetype — its
component-set fingerprint. Query membership by component id, which works for both
components and tags:

```cpp
const ecs::Archetype& arch = world.GetArchetype(entity);
bool frozen = arch.test(ecs::ComponentRegistrator::GetComponentId<Frozen>());
```

For a single tag, prefer `HasTag<T>(entity)` over `GetArchetype` + `test` — it is
the same check, spelled directly and safe on a dead entity (see above).

The reference stays valid until the entity is migrated (any add/remove of components
or tags) or destroyed. `GetArchetype` asserts the entity is alive.

To list *every* tag or *every* component the entity owns instead of testing one at a
time, use `GetTagIds(entity)` / `GetComponentIds(entity)`. Both intersect the
archetype with a process-wide mask kept by `ComponentRegistrator` (one mask of every
registered tag id, one of every registered data-bearing component id) and return the
result as a plain `collections::BitSet` — a one-off snapshot, not a registered
archetype, so it carries no hash and isn't tied to storage:

```cpp
for (ecs::componentId_t id : world.GetTagIds(entity))
    ...                                    // every tag bit the entity has

for (ecs::componentId_t id : world.GetComponentIds(entity))
    ...                                    // every data-bearing component bit
```

Both assert the entity is alive, same as `GetArchetype`.

## Changing an entity's components

`AddComponents` and `RemoveComponents` move the entity to a new archetype,
carrying its existing component values along.

```cpp
// Add one or several components at once:
world.AddComponents(e.getEntity(), Health{50.f}, Damage{15.f});

// Overwrites in place if the component is already present:
world.AddComponents(e.getEntity(), Health{25.f});

// Remove component types:
world.RemoveComponents<Damage>(e.getEntity());
```

Notes:

- Adding a component that already exists **overwrites** it (move-assigned from
  an rvalue, copy-assigned from an lvalue).
- Removing a type the entity doesn't have is ignored.
- Removing the **last** component destroys the entity — an archetype entity
  cannot exist with no components.
- All of these are no-ops on a dead entity.

> **Doing this mid-frame?** Mutating archetypes while a `view` is iterating is
> unsafe. Prefer to defer structural edits with [commands](./commands.md), which
> run between frames.

## Typed wrappers

For a richer API over a fixed component set, derive a wrapper from
`EntityWrapper` (it must add **no** data members — all state lives in
components):

```cpp
struct Player final : ecs::EntityWrapper
{
    using EntityWrapper::EntityWrapper;

    float GetHealth() const         { return GetComponent<Health>().value; }
    void  SetHealth(float v)        { GetComponent<Health>().value = v; }
    Position2d& GetPosition()       { return GetComponent<Position2d>(); }
};

// Ask Create to return your wrapper type:
Player player = world.Create<Player>(std::move(prefab));
player.SetHealth(75.f);
```

The `ecs::EntityWrapperLike` concept enforces the "no extra data members" rule
at compile time.

## See also

- [Components & prefabs](./components-and-prefabs.md)
- [Commands](./commands.md) — deferred create / destroy / add / remove
