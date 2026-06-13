# Компоненты и префабы

[🇬🇧 English](../../en/guides/components-and-prefabs.md) · [⬆ Документация (RU)](../README.md)

## Компоненты — это простые структуры

Компонент — это просто данные: ни базового класса, ни макросов, ни вызова
регистрации. Любой тип удовлетворяет концепту `ecs::IsComponent`.

```cpp
struct Position2d { float x{}, y{}; };
struct Velocity2d { float x{}, y{}; };
struct Health     { float value = 100.f; };
struct Damage     { float value = 10.f; };
```

**ID типа компонента** назначается лениво при первом использовании через
`ecs::ComponentRegistrator`, который также запоминает размер, конструктор,
деструктор и операции перемещения/копирования, чтобы хранилище могло управлять
компонентом обобщённо. Регистратор напрямую вызывать не нужно.

> **Держите компоненты перемещаемыми.** Сущности переносятся между архетипами
> при изменении набора компонентов, поэтому компоненты должны быть
> move-конструируемыми. Компоненты с ресурсами допустимы, если корректно
> перемещаются.

## PrefabEntity — строитель сущности

Сущности не создаются поле за полем. Вместо этого вы наполняете
[`ecs::PrefabEntity`](../api/index_classes.md) компонентами и передаёте его
менеджеру.

```cpp
ecs::PrefabEntity prefab;
prefab.AddComponent<Position2d>(10.f, 20.f);   // аргументы конструктора форвардятся
prefab.AddComponent<Velocity2d>(1.f, 0.f);
prefab.AddComponent<Health>();                 // по умолчанию (100)

ecs::EntityWrapper entity = registry.Entities().Create(prefab);
```

`AddComponent<T>(args...)` конструирует компонент на месте из переданных
аргументов. Повторное добавление того же типа заменяет прежнее значение.

### Переиспользование префаба

Префаб — недолговечный строитель. После `Create` его можно наполнить заново и
использовать снова — удобно для спавна множества похожих сущностей:

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

Префаб берёт слоты компонентов из пула на тип (free list), поэтому повторные
сборки не дёргают аллокатор.

## Вынос построения префаба

Когда один и тот же набор компонентов встречается в нескольких местах, оберните
логику сборки в небольшую структуру. В R-ECS это называется **рецептом**, и
рецепты открывают отложенное создание и события префаба — см.
[Рецепты и «готовка»](./recipes-and-cooking.md).

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

## Добавление и удаление компонентов позже

Набор компонентов не зафиксирован при создании. Архетип сущности можно менять в
любой момент — см.
[Сущности и представления](./entities-and-views.md#изменение-набора-компонентов-сущности)
для `AddComponents` / `RemoveComponents`, или отложите изменение командой (см.
[Команды](./commands.md)).

## См. также

- [Сущности и представления](./entities-and-views.md)
- [Рецепты и «готовка»](./recipes-and-cooking.md)
