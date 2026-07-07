# Commands

[🇷🇺 Русский](../../ru/guides/commands.md) · [⬆ English docs](../README.md)

Commands defer **structural changes** — creating, destroying, or re-shaping
entities — until a safe point between frames. This lets a system request changes
while iterating a `view` without corrupting the storage it is walking.

Queue a command with `registry.Commands().Push(...)`; it executes when the
registry flushes commands during `Update()`.

```cpp
registry.Commands().Push(/* a command */);
// ...nothing happened yet...
registry.Update();   // commands flush here
```

## CreateEntityCmd

Defers entity creation from a prefab, with an optional callback receiving the
new `Entity`:

```cpp
ecs::PrefabEntity prefab;
prefab.AddComponent<Position2d>(3.f, 4.f);

registry.Commands().Push(ecs::CreateEntityCmd{
    std::move(prefab),
    [&](ecs::Entity e) { spawned = e; }      // optional onCreated callback
});

registry.Update();   // entity now exists; callback has fired
```

The prefab may be passed by rvalue (moved) or as a `std::shared_ptr<PrefabEntity>`
when you want to reuse it. The callback is handy for capturing the resulting
handle, because the id isn't known until the flush.

## DeleteEntityCmd

Defers destruction; the optional callback runs **while the entity is still
alive**, just before it is destroyed. Deleting an already-dead entity is
silently skipped.

```cpp
registry.Commands().Push(ecs::DeleteEntityCmd{
    entity,
    [&](ecs::Entity e) { logDeath(e); }      // optional onDeleted callback
});
registry.Update();
```

## AddComponentsCmd

Defers adding (or overwriting) one or more components. Component types are
deduced (CTAD), so you just list values:

```cpp
registry.Commands().Push(ecs::AddComponentsCmd{ entity, ecs::Health{77.f} });

registry.Commands().Push(ecs::AddComponentsCmd{
    entity, Health{50.f}, Damage{15.f}, Speed{3.f}      // several at once
});

registry.Update();
```

Behaviour mirrors `EntitiesManager::AddComponents`: existing components are
overwritten, new ones migrate the entity to a new archetype, and rvalue
arguments are *moved* into place while lvalues are *copied*. A command targeting
a dead entity is a no-op.

> Component values are stored inside the command until the flush, so they must
> be copy-constructible (the queue holds them in a `std::function`).

## RemoveComponentsCmd

Defers removal. The component **types** are template parameters — no values:

```cpp
registry.Commands().Push(ecs::RemoveComponentsCmd<Health>{ entity });
registry.Commands().Push(ecs::RemoveComponentsCmd<Health, Damage>{ entity });
registry.Update();
```

Removing the last remaining component destroys the entity (an archetype entity
cannot be empty).

## CookCmd

Builds an entity from a [recipe](./recipes-and-cooking.md), optionally firing
prefab events. Covered in [Recipes & cooking](./recipes-and-cooking.md):

```cpp
registry.Commands().Push(ecs::CookCmd<Player, PlayerRecipe>{ PlayerRecipe{} });
```

## DisableSystemCmd / EnableSystemCmd

Defers toggling a system on or off. `SystemsManager::Enable` / `Disable` are
main-thread-only, so a system running on a worker thread cannot call them
directly; it pushes one of these commands instead (`CommandQueue::Push` is
thread-safe). The toggle is applied on the Registry's thread at the next flush,
so it lands deterministically rather than racing the running schedule:

```cpp
registry.Commands().Push(ecs::DisableSystemCmd<PhysicsSystem>{});
registry.Commands().Push(ecs::EnableSystemCmd<PhysicsSystem>{});
registry.Update();   // toggle applied during the flush
```

Behaviour mirrors the immediate calls: disabling cascades along `Direct` edges
and also unsubscribes the affected systems' event handlers; enabling clears the
explicit disable and re-subscribes them (subject to the same cascade). See
[Enabling and disabling systems](./systems-and-scheduling.md#enabling-and-disabling-systems).

## Inspecting the queue

```cpp
registry.Commands().size();    // how many commands are pending
```

After a flush the queue is empty again. Commands queued during a flush run on
the next frame, so chains of work spread predictably across frames.

## Pushing from worker threads

`Commands().Push(...)` is callable from any thread, so a system running on a
worker during a parallel stage can defer structural changes freely. There is no
shared lock on the hot path: each worker writes into its **own** bucket (keyed by
the scheduler's `WorkerIndex()`), so concurrent pushes never contend. Pushes from
the main thread between frames take a single, uncontended mutex instead. All
buckets are drained on the Registry's thread at the next flush, worker buckets
first in index order, then the main-thread bucket — a deterministic merge given a
fixed worker assignment. See [Jobs & threading](./jobs.md#threading-rules) for
the full model.

## Commands vs. immediate calls

| | Immediate (`registry.Entities()....`) | Deferred (`registry.Commands().Push(...)`) |
|---|---|---|
| When it runs | now | on the next `Update()` flush |
| Safe during a `view` loop | ✗ (may invalidate iteration) | ✓ |
| Knows the new entity id immediately | ✓ | only via the callback after flush |

## See also

- [Entities & views](./entities-and-views.md#changing-an-entitys-components)
- [Events](./events.md) — defer *messages* rather than structural edits.
