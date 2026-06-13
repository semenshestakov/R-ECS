# Сущности и представления

[🇬🇧 English](../../en/guides/entities-and-views.md) · [⬆ Документация (RU)](../README.md)

Всё, что касается сущностей, идёт через `registry.Entities()`, который
возвращает [`ecs::EntitiesManager`](../api/index_classes.md).

## Создание и уничтожение

```cpp
auto& world = registry.Entities();

ecs::EntityWrapper e = world.Create(prefab);  // собрать из префаба
bool alive = e.IsAlive();                      // true

e.SelfDestroy();                               // уничтожить через обёртку
// или: world.Destroy(e.getEntity());
```

`ecs::Entity` — крошечный хэндл `{id, version}`. Версия увеличивается при
переиспользовании id, поэтому устаревший хэндл на уничтоженную сущность вернёт
`IsAlive() == false`, а не молча «попадёт» в новую сущность.

## EntityWrapper — безопасный хэндл

`Create` возвращает [`ecs::EntityWrapper`](../api/index_classes.md): сущность
плюс ссылку на её менеджер, так что действовать можно напрямую.

```cpp
e.GetComponent<Position2d>().x = 5.f;          // ассерт: компонент существует

if (auto* hp = e.TryGetComponent<Health>())    // nullptr, если нет / мертва
    hp->value -= 10.f;

ecs::entityId_t id = e.getId();
```

`GetComponent<T>()` ассертит, что сущность жива и владеет `T`;
`TryGetComponent<T>()` возвращает указатель (или `nullptr`) и не ассертит.

## Итерация через представления (views)

`view<Components...>()` выдаёт только сущности, владеющие **всеми**
перечисленными компонентами. Каждый шаг итерации разбирается на ссылки, которые
можно читать и менять на месте.

```cpp
for (auto [pos, vel] : world.view<Position2d, Velocity2d>())
{
    pos.x += vel.x;        // ссылки в хранилище — записи сохраняются
    pos.y += vel.y;
}
```

Поскольку сущности сгруппированы по архетипам в плотных чанках, представление
идёт по непрерывной памяти — это кэш-дружелюбное ядро фреймворка. Представление
с одним компонентом работает так же:

```cpp
for (auto [hp] : world.view<Health>())
    if (hp.value <= 0.f) { /* пометить на удаление */ }
```

## Изменение набора компонентов сущности

`AddComponents` и `RemoveComponents` переносят сущность в новый архетип,
сохраняя значения её текущих компонентов.

```cpp
// Добавить один или несколько компонентов сразу:
world.AddComponents(e.getEntity(), Health{50.f}, Damage{15.f});

// Перезаписать на месте, если компонент уже есть:
world.AddComponents(e.getEntity(), Health{25.f});

// Удалить типы компонентов:
world.RemoveComponents<Damage>(e.getEntity());
```

Замечания:

- Добавление уже существующего компонента **перезаписывает** его (move-assign из
  rvalue, copy-assign из lvalue).
- Удаление типа, которого у сущности нет, игнорируется.
- Удаление **последнего** компонента уничтожает сущность — сущность на архетипах
  не может существовать без компонентов.
- Все эти операции — no-op для мёртвой сущности.

> **Делаете это посреди кадра?** Менять архетипы во время итерации `view`
> небезопасно. Лучше откладывайте структурные правки [командами](./commands.md),
> которые выполняются между кадрами.

## Типизированные обёртки

Для более богатого API над фиксированным набором компонентов наследуйте обёртку
от `EntityWrapper` (она не должна добавлять **никаких** полей — всё состояние
живёт в компонентах):

```cpp
struct Player final : ecs::EntityWrapper
{
    using EntityWrapper::EntityWrapper;

    float GetHealth() const         { return GetComponent<Health>().value; }
    void  SetHealth(float v)        { GetComponent<Health>().value = v; }
    Position2d& GetPosition()       { return GetComponent<Position2d>(); }
};

// Попросите Create вернуть ваш тип обёртки:
Player player = world.Create<Player>(std::move(prefab));
player.SetHealth(75.f);
```

Концепт `ecs::EntityWrapperLike` на этапе компиляции проверяет правило «никаких
лишних полей».

## См. также

- [Компоненты и префабы](./components-and-prefabs.md)
- [Команды](./commands.md) — отложенные create / destroy / add / remove
