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

Lead the argument list with `Entity` to also receive the owning entity handle as
the first element of each step. `Entity` is not a component — it is stripped from
the archetype filter, so the view still matches on the remaining components only:

```cpp
for (auto [entity, pos, vel] : world.view<Entity, Position2d, Velocity2d>())
{
    pos.x += vel.x;
    if (pos.x > bound) world.Destroy(entity);   // handle in hand, no extra lookup
}
```

`view<Entity>()` on its own walks every alive entity and yields just its handle.

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
