# События

[🇬🇧 English](../../en/guides/events.md) · [⬆ Документация (RU)](../README.md)

События позволяют системам реагировать на происходящее, не зная друг о друге.
Событие — простая структура; обработчик — функция-член, принимающая реестр и
событие.

## Объявление обработчика через ECS_EVENT

```cpp
struct PlayerDiedEvent { ecs::Entity who; };

struct ScoreSystem final : ecs::ISystem<ScoreSystem>
{
    ECS_REGISTRY("game")

    void OnPlayerDied(ecs::Registry& registry, const PlayerDiedEvent& e)
    {
        score += 100;
    }
    ECS_EVENT(OnPlayerDied, PlayerDiedEvent)   // подписать обработчик

    int score = 0;
};
```

`ECS_EVENT(method, EventType)` подключает `method` как слушателя `EventType`.
Подписка происходит при `registry.Init()`. Система может объявить сколько угодно
обработчиков, и одно событие могут обрабатывать несколько систем.

Сигнатура обработчика всегда такая:

```cpp
void Method(ecs::Registry& registry, const EventType& event);
```

## Два способа вызвать событие

### Немедленно — `OnEvent`

Диспетчеризует синхронно: каждый подписчик выполняется до возврата из `OnEvent`.

```cpp
registry.Events().OnEvent<PlayerDiedEvent>({ .who = id });
```

Используйте, когда эффект нужен сразу (например, внутри системы, которой нужен
результат в этом же кадре).

### Отложенно — `PushEvent`

Ставит событие в очередь; подписчики выполняются позже, в фазе сброса событий
`registry.Update()`. Это исключает побочные эффекты посреди обновления и
сохраняет детерминированный порядок.

```cpp
registry.Events().PushEvent(PlayerDiedEvent{ .who = id });
// ... обработчики выполнятся в следующем registry.Update()
```

```cpp
registry.Events().PushEvent(PlayerDiedEvent{id});
// пока ничего не выполнилось
registry.Update();
// теперь каждый подписчик вызван
```

События, отправленные *во время* сброса, обрабатываются на **следующем** кадре,
а не на текущем — поэтому обработчик может безопасно отправлять последующие
события без рекурсии.

## Порядок следует за DAG систем

При диспетчеризации события подписчики вызываются в том же топологическом
порядке, что и обновления систем. Если `RenderSystem` зависит от
`PhysicsSystem`, то для общего события обработчик `PhysicsSystem` выполнится
раньше `RenderSystem`. Это делает обработку событий такой же детерминированной,
как расписание обновлений (см.
[Системы и планирование](./systems-and-scheduling.md)).

## Разобранный пример — несколько подписчиков

```cpp
struct DamageEvent { ecs::Entity target; float amount; };

struct HealthSystem final : ecs::ISystem<HealthSystem>
{
    ECS_REGISTRY("game")
    void OnDamage(ecs::Registry& r, const DamageEvent& e)
    {
        if (auto* hp = /* найти e.target */ nullptr) hp->value -= e.amount;
    }
    ECS_EVENT(OnDamage, DamageEvent)
};

struct AudioSystem final : ecs::ISystem<AudioSystem>
{
    ECS_REGISTRY("game")
    void OnDamage(ecs::Registry& r, const DamageEvent& e) { playHitSound(); }
    ECS_EVENT(OnDamage, DamageEvent)
};

// Где угодно, где есть реестр:
registry.Events().PushEvent(DamageEvent{ target, 25.f });
```

И `HealthSystem`, и `AudioSystem` получают каждое `DamageEvent` в порядке DAG.

## События префаба

Две встроенные полезные нагрузки, `ecs::PrefabEvt<E>` и
`ecs::CreatedEntityEvt<E>`, вызываются вокруг создания сущности по рецепту, чтобы
системы могли обогатить префаб или отреагировать на свежесозданную сущность. См.
[Рецепты и «готовка»](./recipes-and-cooking.md).

## См. также

- [Команды](./commands.md) — откладывают *структурные* изменения (события
  откладывают *сообщения*).
- [Системы и планирование](./systems-and-scheduling.md) — порядок, который
  наследуют события.
