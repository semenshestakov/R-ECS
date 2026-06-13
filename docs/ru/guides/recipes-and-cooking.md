# Рецепты и «готовка»

[🇬🇧 English](../../en/guides/recipes-and-cooking.md) · [⬆ Документация (RU)](../README.md)

**Рецепт** описывает, как наполнить префаб для конкретного вида сущности.
**Готовка** — это создание сущности из рецепта, при желании с оповещением через
события, чтобы другие системы могли вмешаться.

## Рецепт — это всё, у чего есть `apply`

Концепт `ecs::Recipe` принимает любой тип, у которого есть
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

Рецепт можно применить напрямую:

```cpp
ecs::PrefabEntity prefab;
PlayerRecipe::apply(prefab);
Player player = registry.Entities().Create<Player>(std::move(prefab));
```

## CookCmd — отложенное создание из рецепта

`ecs::CookCmd<EntityType, Recipe>` — это [команда](./commands.md): она применяет
рецепт и создаёт сущность во время сброса. `EntityType` — тип обёртки для сборки
(`Entity` или производный от `EntityWrapper`, например `Player`).

```cpp
registry.Commands().Push(ecs::CookCmd<Player, PlayerRecipe>{ PlayerRecipe{} });
registry.Commands().Push(ecs::CookCmd<Enemy,  EnemyRecipe>{  EnemyRecipe{}  });

registry.Update();   // обе сущности созданы во время сброса
```

В одном кадре можно поставить в очередь много «готовок» разных типов; каждая
порождает одну сущность.

## События префаба — пусть системы вмешаются

`CookCmd` может вызвать два события, управляемых флагами `ecs::CookFeedback`:

| Флаг | Вызываемое событие | Когда |
|------|--------------------|-------|
| `CookFeedback::PRE_EVT_CALL` | `ecs::PrefabEvt<E>` | **до** создания — обработчики могут обогатить префаб |
| `CookFeedback::POST_EVT_CALL` | `ecs::CreatedEntityEvt<E>` | **после** создания — обработчики видят живую сущность |
| `CookFeedback::NONE` | — | по умолчанию, без событий |

Комбинируйте флаги через `|`:

```cpp
registry.Commands().Push(ecs::CookCmd<Player, PlayerRecipe>{
    PlayerRecipe{},
    ecs::CookFeedback::PRE_EVT_CALL | ecs::CookFeedback::POST_EVT_CALL
});
```

### Обогащение префаба до создания

Система подписывается на `PrefabEvt<Player>` и добавляет в префаб ещё
компоненты — каждый игрок получает дополнительный компонент, а рецепт об этом не
знает:

```cpp
struct PlayerEnricher final : ecs::ISystem<PlayerEnricher>
{
    ECS_REGISTRY("game")

    void OnPrefab(ecs::Registry& r, const ecs::PrefabEvt<Player>& e)
    {
        e.prefab.AddComponent<TestId>(99);     // меняем префаб на месте
    }
    ECS_EVENT(OnPrefab, ecs::PrefabEvt<Player>)
};
```

### Реакция на свежесозданную сущность

Система подписывается на `CreatedEntityEvt<Player>` и работает с живой обёрткой:

```cpp
struct PlayerSpawnLogger final : ecs::ISystem<PlayerSpawnLogger>
{
    ECS_REGISTRY("game")

    void OnCreated(ecs::Registry& r, const ecs::CreatedEntityEvt<Player>& e)
    {
        // e.entity — живая обёртка Player
        assert(e.entity.IsAlive());
    }
    ECS_EVENT(OnCreated, ecs::CreatedEntityEvt<Player>)
};
```

Оба события типизированы по обёртке сущности, поэтому обработчик
`PrefabEvt<Player>` никогда не получит `CookCmd<Camera, ...>` и наоборот.

## Собираем вместе

```cpp
// 1. PlayerEnricher и PlayerSpawnLogger зарегистрированы под "game".
ecs::Registry registry = ecs::Registry::Create("game");
registry.Init();

// 2. «Готовим» игрока с обоими событиями.
registry.Commands().Push(ecs::CookCmd<Player, PlayerRecipe>{
    PlayerRecipe{},
    ecs::CookFeedback::PRE_EVT_CALL | ecs::CookFeedback::POST_EVT_CALL
});

// 3. При сбросе: вызывается PrefabEvt (enricher добавляет TestId), создаётся
//    сущность, затем вызывается CreatedEntityEvt (logger её осматривает).
registry.Update();
```

## См. также

- [Команды](./commands.md) — механизм отложенности, на котором ездит `CookCmd`.
- [События](./events.md) — как работают подписки `ECS_EVENT`.
- [Компоненты и префабы](./components-and-prefabs.md) — что собирает рецепт.
