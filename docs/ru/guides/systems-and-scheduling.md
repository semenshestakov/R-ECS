# Системы и планирование

[🇬🇧 English](../../en/guides/systems-and-scheduling.md) · [⬆ Документация (RU)](../README.md)

## Определение системы

Система наследует `ecs::ISystem<Derived>` (CRTP) и переопределяет `Update`:

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

Наследование `ISystem<PhysicsSystem>` делает две вещи автоматически:

- регистрирует тип системы в `ecs::SystemRegistrator` при статической
  инициализации (ручной вызов не нужен), и
- даёт механику `ECS_EVENT` / `ECS_DEPENDENT_SYSTEMS`.

## ECS_REGISTRY — имя системы

`ECS_REGISTRY("name", ...)` объявляет имя(имена) реестра, к которому принадлежит
система. Все системы с одним именем связываются вместе при вызове
`Registry::Create("name")`:

```cpp
ecs::Registry registry = ecs::Registry::Create("game");
registry.Init();
```

Система может принадлежать нескольким реестрам: `ECS_REGISTRY("game", "editor")`.

## Объявление зависимостей — DAG

Системы выполняются в **топологическом порядке** из графа зависимостей.
Объявите, после чего система должна выполняться, через `ECS_DEPENDENT_SYSTEMS`:

```cpp
struct ResourceSystem final : ecs::ISystem<ResourceSystem> { ECS_REGISTRY("game") /* ... */ };
struct InputSystem    final : ecs::ISystem<InputSystem>    { ECS_REGISTRY("game") /* ... */ };
struct PhysicsSystem  final : ecs::ISystem<PhysicsSystem>  { ECS_REGISTRY("game") /* ... */ };

struct RenderSystem final : ecs::ISystem<RenderSystem>
{
    ECS_REGISTRY("game")
    ECS_DEPENDENT_SYSTEMS(ResourceSystem, InputSystem, PhysicsSystem)

    void Update(ecs::Registry&, const ecs::UpdateState&) override { /* отрисовка */ }
};

struct PostRenderSystem final : ecs::ISystem<PostRenderSystem>
{
    ECS_REGISTRY("game")
    ECS_DEPENDENT_SYSTEMS(RenderSystem)   // строго после RenderSystem

    void Update(ecs::Registry&, const ecs::UpdateState&) override { /* present */ }
};
```

При таком объявлении расписание гарантирует:

```
ResourceSystem ┐
InputSystem    ├─▶ RenderSystem ─▶ PostRenderSystem
PhysicsSystem  ┘
```

`ResourceSystem`, `InputSystem` и `PhysicsSystem` не упорядочены между собой —
учитываются только объявленные рёбра. Граф — это
[`collections::DirectedAcyclicGraph`](../api/index_classes.md), и цикл является
ошибкой настройки.

### Два вида рёбер зависимости

Каждое ребро расписания несёт свой вид (`ecs::SystemDep`):

- `Direct` — **жёсткая** зависимость из `ECS_DEPENDENT_SYSTEMS`. Задаёт порядок
  выполнения *и* распространяет выключение (см. ниже).
- `Data` — **слабая** зависимость. Задаёт порядок выполнения, но никогда не
  распространяет выключение. `Data`-рёбра берутся из объявленного доступа к
  компонентам и из `ECS_WEAK_DEPENDENT_SYSTEMS`.

Одно ребро может быть обоих видов сразу (флаги объединяются по OR).

### Когда пересчитывается расписание

Добавление системы **не** пересчитывает расписание. `Add` лишь регистрирует
систему и помечает расписание «грязным» (dirty); топологическая сортировка и
вывод data-рёбер выполняются лениво при следующем `Build()`, который менеджер
систем вызывает в начале `Update()` (и `Subscribe()`). Поэтому регистрация систем
дешёвая, а пересчёт зависимостей происходит один раз во время обновления, а не на
каждый `Add`. Если расписание не «грязное», `Build()` — это no-op.

## Доступ к компонентам — `ECS_ACCESS`

Вместо жёстких рёбер (или вместе с ними) система может объявить, какие компоненты
она читает и пишет. Расписание автоматически превращает это в порядок:

```cpp
struct MovementSystem final : ecs::ISystem<MovementSystem>
{
    ECS_REGISTRY("game")
    ECS_ACCESS(ecs::ReadWrite<Position2d>, ecs::Read<Velocity2d>)

    void Update(ecs::Registry&, const ecs::UpdateState&) override { /* ... */ }
};
```

Теги доступа: `ecs::Read<C>` (RO), `ecs::Write<C>` (WO) и `ecs::ReadWrite<C>`
(RW). Из объявленного доступа расписание выводит `Data`-рёбра так, что:

- каждый **читатель** компонента выполняется после каждого его **писателя**
  (writer-before-reader), и
- два **писателя** одного компонента упорядочены детерминированно (первым идёт
  система с меньшим хешем).

Эти выведенные рёбра ограничивают только порядок — они никогда не приводят к
выключению системы при выключении другой.

## Слабые зависимости — `ECS_WEAK_DEPENDENT_SYSTEMS`

Используйте слабую зависимость, когда система должна выполняться *после* другой,
но обязана продолжать работать, даже если та выключена:

```cpp
struct HudSystem final : ecs::ISystem<HudSystem>
{
    ECS_REGISTRY("game")
    ECS_WEAK_DEPENDENT_SYSTEMS(ScoreSystem)   // после ScoreSystem, но независимо

    void Update(ecs::Registry&, const ecs::UpdateState&) override { /* ... */ }
};
```

Это добавляет `Data`-ребро: порядок соблюдается, но выключение `ScoreSystem`
оставляет `HudSystem` работающей.

## Включение и выключение систем

Системы можно включать и выключать во время выполнения через менеджер систем:

```cpp
registry.Systems().Disable<PhysicsSystem>();
bool on = registry.Systems().IsEnabled<PhysicsSystem>();
registry.Systems().Enable<PhysicsSystem>();
```

Выключенная система перестаёт получать вызовы `Update()` (и `Subscribe()`).
Выключение **распространяется по `Direct`-рёбрам транзитивно**: каждая система,
которая жёстко зависит от выключенной — напрямую или по цепочке — тоже
пропускается. Системы, связанные только доступом к компонентам или через
`ECS_WEAK_DEPENDENT_SYSTEMS`, продолжают работать.

```
            disable ─┐
ResourceSystem       ▼
InputSystem ─▶ RenderSystem ─▶ PostRenderSystem   (обе пропущены: жёсткая цепочка)
PhysicsSystem
HudSystem ⇢ RenderSystem                          (слабое ребро: продолжает работать)
```

`Enable` снимает только явное выключение; система остаётся неактивной, пока она
жёстко зависит от чего-то выключенного. `IsEnabled` отражает весь каскад.

## Доступ к другим системам и общему состоянию

Внутри `Update` (или где угодно, где есть `Registry&`):

```cpp
auto& physics = registry.Systems().Get<PhysicsSystem>();   // по типу
auto* maybe   = registry.Systems().TryGet<AudioSystem>();  // nullptr, если нет

// Общие синглтоны по типу живут в контексте:
auto& clock = registry.ctx().getOrEmplace<GameClock>();
```

[`collections::Context`](../api/index_classes.md) (`registry.ctx()`) хранит по
одному экземпляру на тип — идеально для ресурсов и сервисов, общих для систем.

## Ручная регистрация (без имён)

Если не хотите использовать именованные реестры, соберите реестр вручную:

```cpp
ecs::Registry registry;                 // пустой
registry.Systems().Register<PhysicsSystem>();
registry.Systems().Register<RenderSystem>();
registry.Init();
```

## См. также

- [События](./events.md) — обработчики событий систем тоже следуют порядку DAG.
- [Быстрый старт](./getting-started.md#цикл-кадра) — где `Update` в кадре.
