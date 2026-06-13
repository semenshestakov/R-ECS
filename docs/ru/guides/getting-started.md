# Быстрый старт

[🇬🇧 English](../../en/guides/getting-started.md) · [⬆ Документация (RU)](../README.md)

## Подключение R-ECS к сборке

R-ECS — статическая библиотека с интерфейсом через CMake. Поместите репозиторий
в проект (submodule, `FetchContent` или vendored-папка) и слинкуйте:

```cmake
add_subdirectory(R-ECS)

target_link_libraries(my_game PRIVATE R-ECS)
```

Тесты и бенчмарки собираются **только** когда R-ECS — корневой проект, поэтому
подключение как подкаталога оставляет вашу сборку лёгкой. Требуется компилятор
C++20.

## Ментальная модель

В R-ECS четыре движущиеся части, и все принадлежат единственному
`ecs::Registry`:

- **Компоненты** — простые структуры с данными (без логики).
- **Сущности** — лёгкие хэндлы `{id, version}`, владеющие набором компонентов
  (своим *архетипом*).
- **Системы** — поведение. Каждый кадр реестр вызывает `Update` у каждой системы
  в порядке зависимостей.
- **События и команды** — типобезопасные очереди для межсистемных сообщений и
  отложенных структурных изменений.

## Первая программа

```cpp
#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/entities/PrefabEntity.hpp"

struct Position2d { float x, y; };
struct Velocity2d { float x, y; };

// Система: регистрируем под именем и реализуем Update().
struct MoveSystem final : ecs::ISystem<MoveSystem>
{
    ECS_REGISTRY("game")          // связывает систему с реестром "game"

    void Update(ecs::Registry& registry, const ecs::UpdateState&) override
    {
        for (auto [pos, vel] : registry.Entities().view<Position2d, Velocity2d>())
        {
            pos.x += vel.x;
            pos.y += vel.y;
        }
    }
};

int main()
{
    // Реестр, заранее заполненный всеми системами с именем "game".
    ecs::Registry registry = ecs::Registry::Create("game");
    registry.Init();

    // Собираем сущность из префаба.
    ecs::PrefabEntity prefab;
    prefab.AddComponent<Position2d>(0.f, 0.f);
    prefab.AddComponent<Velocity2d>(1.f, 2.f);

    for (int i = 0; i < 100; ++i)
        registry.Entities().Create(prefab);

    // Главный цикл.
    for (int frame = 0; frame < 60; ++frame)
        registry.Update();
}
```

> **Два способа создать `Registry`.** `Registry::Create("game")` находит все
> системы с меткой `ECS_REGISTRY("game")` и связывает их автоматически. Можно
> также создать пустой `ecs::Registry registry;` и регистрировать системы
> вручную через `registry.Systems()`.

## Цикл кадра

`registry.Update()` выполняет один кадр строго в таком порядке:

1. **Сброс команд**, поставленных в очередь *до* кадра.
2. **Обновление всех систем** в топологическом (по зависимостям) порядке.
3. **Сброс событий** (`PushEvent`) подписчикам.
4. **Сброс команд**, поставленных *во время* кадра.

```cpp
while (running)
{
    pollInput();
    registry.Update();   // системы + события + команды
    render();
}
```

`registry.Init()` нужно вызвать один раз до первого `Update()`: он
инициализирует системы и подписывает их обработчики событий.

## Что дальше

- [Компоненты и префабы](./components-and-prefabs.md) — описать данные и собрать сущности.
- [Сущности и представления](./entities-and-views.md) — запрашивать и менять сущности.
- [Системы и планирование](./systems-and-scheduling.md) — упорядочить системы через DAG.
- [События](./events.md) и [Команды](./commands.md) — общаться и откладывать работу.
