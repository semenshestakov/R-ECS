# Быстрый старт

[🇬🇧 English](../../en/guides/getting-started.md) · [⬆ Документация (RU)](../README.md)

## Подключение R-ECS к сборке

R-ECS — **header-only** библиотека с интерфейсом через CMake. Поместите
репозиторий в проект (submodule, `FetchContent` или vendored-папка) и слинкуйте:

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
#include <cstddef>
#include <iostream>

#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/entities/PrefabEntity.hpp"

struct Vel2d      { float x, y; };
struct Position2d { float x, y; };

// Первая система: интегрирует скорость и применяет затухание.
struct MoveSystem final : ecs::ISystem<MoveSystem>
{
    ECS_REGISTRY("MyName")

    void Update(ecs::Registry& registry, const ecs::UpdateState&) override
    {
        for (auto [pos, vel] : registry.Entities().view<Position2d, Vel2d>())
        {
            pos.x += vel.x;
            pos.y += vel.y;

            vel.x *= 0.98f;
            vel.y *= 0.98f;
        }
    }
};

// Вторая система: логирует позиции — гарантированно запускается после MoveSystem.
struct LogMoveSystem final : ecs::ISystem<LogMoveSystem>
{
    ECS_DEPENDENT_SYSTEMS(MoveSystem)
    ECS_REGISTRY("MyName")

    void Update(ecs::Registry& registry, const ecs::UpdateState&) override
    {
        std::cout << "[Logger] Move system started\n";
        std::size_t i = 0;
        for (auto [pos, vel] : registry.Entities().view<Position2d, Vel2d>())
            std::cout << "[" << ++i << "] \t" << pos.x << ", " << pos.y << "\n";
    }
};

int main()
{
    // Реестр, заранее заполненный всеми системами с именем "MyName".
    auto registry = ecs::Registry::Create("MyName");
    registry.Init();

    // Собираем сущности из префаба, затем переопределяем позицию каждой.
    ecs::PrefabEntity prefab;
    prefab.AddComponent<Position2d>(0.f, 0.f);
    prefab.AddComponent<Vel2d>(100.f, 100.f);

    for (std::size_t i = 0; i < 100; ++i)
    {
        ecs::EntityWrapper entity = registry.Entities().Create(prefab);
        entity.GetComponent<Position2d>().x =  static_cast<float>(i);
        entity.GetComponent<Position2d>().y = -static_cast<float>(i);
    }

    // Главный цикл.
    for (std::size_t i = 0; i < 100; ++i)
    {
        std::cout << "Frame:" << i << "\n";
        registry.Update();
    }
}
```

> **Два способа создать `Registry`.** `Registry::Create("MyName")` находит все
> системы с меткой `ECS_REGISTRY("MyName")` и связывает их автоматически. Можно
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
