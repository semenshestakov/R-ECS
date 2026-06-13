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
