# Recipes & cooking

[🇷🇺 Русский](../../ru/guides/recipes-and-cooking.md) · [⬆ English docs](../README.md)

A **recipe** captures how to populate a prefab for a particular kind of entity.
**Cooking** is creating an entity from a recipe — optionally announcing it
through events so other systems can join in.

## A recipe is anything with `apply`

The `ecs::Recipe` concept accepts any type exposing
`apply(ecs::PrefabEntity&)`:

```cpp
struct PlayerRecipe
{
    static void apply(ecs::PrefabEntity& p)
    {
        p.AddComponent<Health>(100.f);
        p.AddComponent<Position2d>(0.f, 0.f);
    }
};

struct EnemyRecipe
{
    static void apply(ecs::PrefabEntity& p)
    {
        p.AddComponent<Health>(50.f);
        p.AddComponent<Damage>(15.f);
        p.AddComponent<Position2d>(10.f, 10.f);
    }
};
```

You can apply a recipe directly:

```cpp
ecs::PrefabEntity prefab;
PlayerRecipe::apply(prefab);
Player player = registry.Entities().Create<Player>(std::move(prefab));
```

## CookCmd — deferred creation from a recipe

`ecs::CookCmd<EntityType, Recipe>` is a [command](./commands.md): it applies the
recipe and creates the entity at flush time. `EntityType` is the wrapper type to
build (`Entity`, or an `EntityWrapper`-derived type such as `Player`).

```cpp
registry.Commands().Push(ecs::CookCmd<Player, PlayerRecipe>{ PlayerRecipe{} });
registry.Commands().Push(ecs::CookCmd<Enemy,  EnemyRecipe>{  EnemyRecipe{}  });

registry.Update();   // both entities created during the flush
```

You can queue many cooks of mixed types in a single frame; each produces one
entity.

## Prefab events — let systems join in

`CookCmd` can emit two events, controlled by `ecs::CookFeedback` flags:

| Flag | Event fired | When |
|------|-------------|------|
| `CookFeedback::PRE_EVT_CALL` | `ecs::PrefabEvt<E>` | **before** creation — handlers may enrich the prefab |
| `CookFeedback::POST_EVT_CALL` | `ecs::CreatedEntityEvt<E>` | **after** creation — handlers see the live entity |
| `CookFeedback::NONE` | — | default, no events |

Combine flags with `|`:

```cpp
registry.Commands().Push(ecs::CookCmd<Player, PlayerRecipe>{
    PlayerRecipe{},
    ecs::CookFeedback::PRE_EVT_CALL | ecs::CookFeedback::POST_EVT_CALL
});
```

### Enriching a prefab before creation

A system subscribes to `PrefabEvt<Player>` and adds more components to the
prefab — every player gains an extra component without the recipe knowing:

```cpp
struct PlayerEnricher final : ecs::ISystem<PlayerEnricher>
{
    ECS_REGISTRY("game")

    void OnPrefab(ecs::Registry& r, const ecs::PrefabEvt<Player>& e)
    {
        e.prefab.AddComponent<TestId>(99);     // mutate the prefab in place
    }
    ECS_EVENT(OnPrefab, ecs::PrefabEvt<Player>)
};
```

### Reacting to a freshly created entity

A system subscribes to `CreatedEntityEvt<Player>` and works with the live
wrapper:

```cpp
struct PlayerSpawnLogger final : ecs::ISystem<PlayerSpawnLogger>
{
    ECS_REGISTRY("game")

    void OnCreated(ecs::Registry& r, const ecs::CreatedEntityEvt<Player>& e)
    {
        // e.entity is a live Player wrapper
        assert(e.entity.IsAlive());
    }
    ECS_EVENT(OnCreated, ecs::CreatedEntityEvt<Player>)
};
```

Both events are typed on the entity wrapper, so a `PrefabEvt<Player>` handler
never sees a `CookCmd<Camera, ...>` and vice-versa.

## Putting it together

```cpp
// 1. PlayerEnricher and PlayerSpawnLogger are registered under "game".
ecs::Registry registry = ecs::Registry::Create("game");
registry.Init();

// 2. Cook a player with both events enabled.
registry.Commands().Push(ecs::CookCmd<Player, PlayerRecipe>{
    PlayerRecipe{},
    ecs::CookFeedback::PRE_EVT_CALL | ecs::CookFeedback::POST_EVT_CALL
});

// 3. On flush: PrefabEvt fires (enricher adds TestId), the entity is created,
//    then CreatedEntityEvt fires (logger inspects it).
registry.Update();
```

## See also

- [Commands](./commands.md) — the deferral mechanism `CookCmd` rides on.
- [Events](./events.md) — how `ECS_EVENT` subscriptions work.
- [Components & prefabs](./components-and-prefabs.md) — what a recipe assembles.
