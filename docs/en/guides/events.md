# Events

[🇷🇺 Русский](../../ru/guides/events.md) · [⬆ English docs](../README.md)

Events let systems react to things without knowing about each other. An event is
a plain struct; a handler is a member function that takes the registry and the
event.

## Declaring a handler with ECS_EVENT

```cpp
struct PlayerDiedEvent { ecs::entityId_t who; };

struct ScoreSystem final : ecs::ISystem<ScoreSystem>
{
    ECS_REGISTRY("game")

    void OnPlayerDied(ecs::Registry& registry, const PlayerDiedEvent& e)
    {
        score += 100;
    }
    ECS_EVENT(OnPlayerDied, PlayerDiedEvent)   // subscribe the handler

    int score = 0;
};
```

`ECS_EVENT(method, EventType)` wires `method` as a listener for `EventType`. The
subscription happens during `registry.Init()`. A system may declare any number
of handlers, and several systems may handle the same event.

The handler signature is always:

```cpp
void Method(ecs::Registry& registry, const EventType& event);
```

## Two ways to fire an event

### Immediate — `OnEvent`

Dispatches synchronously: every subscriber runs before `OnEvent` returns.

```cpp
registry.Events().OnEvent<PlayerDiedEvent>({ .who = id });
```

Use it when you want the effect to be visible right away (e.g. inside a system
that needs the result this frame).

### Deferred — `PushEvent`

Queues the event; subscribers run later, during the event-flush phase of
`registry.Update()`. This avoids mid-update side effects and keeps ordering
deterministic.

```cpp
registry.Events().PushEvent(PlayerDiedEvent{ .who = id });
// ... handlers run during the next registry.Update()
```

```cpp
registry.Events().PushEvent(PlayerDiedEvent{id});
// nothing has run yet
registry.Update();
// now every subscriber has been called
```

Events pushed *during* a flush are processed on the **next** frame, not the
current one — so a handler can safely push follow-up events without recursing.

## Ordering follows the system DAG

When an event is dispatched, subscribers are invoked in the same topological
order as system updates. If `RenderSystem` depends on `PhysicsSystem`, then for a
shared event `PhysicsSystem`'s handler runs before `RenderSystem`'s. This makes
event handling as deterministic as the update schedule
(see [Systems & scheduling](./systems-and-scheduling.md)).

## Worked example — several subscribers

```cpp
struct DamageEvent { ecs::entityId_t target; float amount; };

struct HealthSystem final : ecs::ISystem<HealthSystem>
{
    ECS_REGISTRY("game")
    void OnDamage(ecs::Registry& r, const DamageEvent& e)
    {
        if (auto* hp = /* look up e.target */ nullptr) hp->value -= e.amount;
    }
    ECS_EVENT(OnDamage, DamageEvent)
};

struct AudioSystem final : ecs::ISystem<AudioSystem>
{
    ECS_REGISTRY("game")
    void OnDamage(ecs::Registry& r, const DamageEvent& e) { playHitSound(); }
    ECS_EVENT(OnDamage, DamageEvent)
};

// Anywhere with the registry:
registry.Events().PushEvent(DamageEvent{ target, 25.f });
```

Both `HealthSystem` and `AudioSystem` receive every `DamageEvent`, in DAG order.

## Prefab events

Two built-in event payloads, `ecs::PrefabEvt<E>` and `ecs::CreatedEntityEvt<E>`,
are fired around recipe-based entity creation so systems can enrich a prefab or
react to a freshly built entity. See
[Recipes & cooking](./recipes-and-cooking.md).

## See also

- [Commands](./commands.md) — defer *structural* changes (events defer *messages*).
- [Systems & scheduling](./systems-and-scheduling.md) — the ordering events inherit.
