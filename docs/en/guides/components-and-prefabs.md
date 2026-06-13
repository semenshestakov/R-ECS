# Components & prefabs

[🇷🇺 Русский](../../ru/guides/components-and-prefabs.md) · [⬆ English docs](../README.md)

## Components are plain structs

A component is just data — no base class, no macros, no registration call. Any
type satisfies the `ecs::IsComponent` concept.

```cpp
struct Position2d { float x{}, y{}; };
struct Velocity2d { float x{}, y{}; };
struct Health     { float value = 100.f; };
struct Damage     { float value = 10.f; };
```

Component **type IDs** are assigned lazily on first use by
`ecs::ComponentRegistrator`, which also records the size, constructor,
destructor and move/copy operations so the storage can manage your component
generically. You never call the registrator directly.

> **Keep components movable.** Entities are relocated between archetypes when
> their component set changes, so components should be move-constructible.
> Components that own resources are fine as long as they move correctly.

## PrefabEntity — the entity builder

You don't create entities field by field. Instead you fill a
[`ecs::PrefabEntity`](../api/index_classes.md) with components, then hand it to
the manager.

```cpp
ecs::PrefabEntity prefab;
prefab.AddComponent<Position2d>(10.f, 20.f);   // ctor args are forwarded
prefab.AddComponent<Velocity2d>(1.f, 0.f);
prefab.AddComponent<Health>();                 // default-constructed (100)

ecs::EntityWrapper entity = registry.Entities().Create(prefab);
```

`AddComponent<T>(args...)` constructs the component in place from the forwarded
arguments. Adding the same component type twice replaces the earlier value.

### Reusing a prefab

A prefab is a short-lived builder. After `Create`, it can be refilled and used
again — handy for spawning many similar entities:

```cpp
ecs::PrefabEntity bullet;
bullet.AddComponent<Position2d>();
bullet.AddComponent<Velocity2d>(0.f, -500.f);

for (int i = 0; i < 50; ++i)
{
    auto e = registry.Entities().Create(bullet);
    e.GetComponent<Position2d>().x = static_cast<float>(i * 8);
}
```

The prefab pulls component slots from a per-type free list, so repeated builds
avoid hitting the allocator.

## Factoring prefab construction

When the same component set appears in several places, wrap the building logic
in a small struct. R-ECS calls this a **recipe**, and recipes unlock deferred
creation and prefab events — see
[Recipes & cooking](./recipes-and-cooking.md).

```cpp
struct PlayerRecipe
{
    static void apply(ecs::PrefabEntity& p)
    {
        p.AddComponent<Health>(100.f);
        p.AddComponent<Position2d>(0.f, 0.f);
    }
};

ecs::PrefabEntity prefab;
PlayerRecipe::apply(prefab);
auto player = registry.Entities().Create(prefab);
```

## Adding or removing components later

Component sets are not fixed at creation. You can change an entity's archetype
at any time — see [Entities & views](./entities-and-views.md#changing-an-entitys-components)
for `AddComponents` / `RemoveComponents`, or defer the change with a command
(see [Commands](./commands.md)).

## See also

- [Entities & views](./entities-and-views.md)
- [Recipes & cooking](./recipes-and-cooking.md)
