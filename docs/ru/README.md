# R-ECS — Документация (русский)

[🇬🇧 English](../en/README.md) · [⬆ Главная docs](../README.md) · [📦 Репозиторий](../../README.md)

R-ECS — это **Entity-Component-System фреймворк на архетипах** для современного
C++ (C++20). Сущности хранятся в плотных чанках, сгруппированных по набору
компонентов (архетипу), что даёт дружелюбную к кэшу итерацию; системы
планируются через граф зависимостей (DAG); события и структурные изменения
проходят через типобезопасные очереди.

## 🚀 С чего начать

| Руководство | О чём                                                                    |
|-------------|--------------------------------------------------------------------------|
| [Быстрый старт](./guides/getting-started.md) | Сборка, первая программа на 30 строк, цикл кадра                         |
| [Компоненты и префабы](./guides/components-and-prefabs.md) | Описание компонентов, `PrefabEntity`, `AddComponent`                     |
| [Сущности и представления](./guides/entities-and-views.md) | `Create`, `GetComponent`, `view<...>`, `EntityWrapper`                   |
| [Системы и планирование](./guides/systems-and-scheduling.md) | `ISystem`, `ECS_REGISTRY`, `ECS_DEPENDENT_SYSTEMS`, DAG                  |
| [События](./guides/events.md) | `ECS_EVENT`, `OnEvent` против `PushEvent`, приоритет                     |
| [Команды](./guides/commands.md) | Отложенные `CreateEntityCmd` / `DeleteEntityCmd` / `AddComponentsCmd` / `RemoveComponentsCmd` |
| [Рецепты и «готовка»](./guides/recipes-and-cooking.md) | `Recipe`, `CookCmd`, `PrefabEvt` / `CreatedEntityEvt`                    |
| [Джобы и многопоточность](./guides/jobs.md) | `IJobScheduler`, `SetScheduler`, подключаемые параллельные бэкенды        |

## 📚 Справочник API

Генерируется из докстрингов исходников (сначала запустите
[`docs/scripts/generate-docs.sh`](../scripts/generate-docs.sh)):

- [Классы](./api/index_classes.md)
- [Пространства имён](./api/index_namespaces.md)
- [Файлы](./api/index_files.md)

## 🗺 Карта библиотеки — по директориям `include/`

### `include/ecs` — основной фасад

| Заголовок | Назначение | Руководство |
|-----------|------------|-------------|
| `ecs/Registry.hpp` | Центральный узел: владеет сущностями, событиями, системами, контекстом, очередью команд | [Быстрый старт](./guides/getting-started.md) |
| `ecs/ISystem.hpp` | CRTP-база для систем и макросы `ECS_*` | [Системы и планирование](./guides/systems-and-scheduling.md) |

### `include/ecs/entities` — сущности и хранилище

| Заголовок | Назначение | Руководство |
|-----------|------------|-------------|
| `entities/EntitiesManager.hpp` | Жизненный цикл сущностей: `Create` / `Destroy` / `view` / `AddComponents` | [Сущности и представления](./guides/entities-and-views.md) |
| `entities/PrefabEntity.hpp` | Строитель, собирающий данные компонентов до создания | [Компоненты и префабы](./guides/components-and-prefabs.md) |
| `entities/EntityWrapper.hpp` | Безопасный хэндл (`IsAlive`, `GetComponent`, `SelfDestroy`) | [Сущности и представления](./guides/entities-and-views.md) |
| `entities/Archetype.hpp` | «Отпечаток» набора компонентов для группировки | [Сущности и представления](./guides/entities-and-views.md) |
| `entities/ArchetypedChunks.hpp` | Плотное чанковое хранилище и итераторы представлений | — |
| `entities/EntitiesArchetypeStorage.hpp` | Отображение архетип → хранилище чанков | — |
| `entities/Entity.hpp` | Хэндл `{id, version}` | — |

### `include/ecs/components` — регистрация компонентов

| Заголовок | Назначение | Руководство |
|-----------|------------|-------------|
| `components/ComponentRegistrator.hpp` | Назначает стабильные ID компонентов, хранит ctor/dtor/move | [Компоненты и префабы](./guides/components-and-prefabs.md) |
| `components/ComponentFreeList.hpp` | Пул слотов на тип, переиспользуемый префабами | — |
| `components/Utils.hpp` | `IsComponent`, `componentId_t`, лимиты | [Компоненты и префабы](./guides/components-and-prefabs.md) |

### `include/ecs/systems` — планирование и диспетчеризация

| Заголовок | Назначение | Руководство |
|-----------|------------|-------------|
| `systems/SystemsManager.hpp` | Владеет экземплярами систем, вызывает `Init` / `Update` / `Subscribe` | [Системы и планирование](./guides/systems-and-scheduling.md) |
| `systems/SystemsSchedule.hpp` | Топологический порядок из графа зависимостей | [Системы и планирование](./guides/systems-and-scheduling.md) |
| `systems/EventSystem.hpp` | Мост событий ECS: `OnEvent` / `PushEvent` / `FlushEvents` | [События](./guides/events.md) |
| `systems/SystemRegistrator.hpp` | Статическая авто-регистрация типов систем | [Системы и планирование](./guides/systems-and-scheduling.md) |
| `systems/IBaseSystem.hpp` | Нешаблонный интерфейс системы | — |

### `include/ecs/jobs` — порт многопоточности

| Заголовок | Назначение | Руководство |
|-----------|------------|-------------|
| `jobs/IJobScheduler.hpp` | Абстрактный потоковый бэкенд (`ParallelFor`, `Run`, `Wait`) | [Джобы и многопоточность](./guides/jobs.md) |
| `jobs/SerialJobScheduler.hpp` | Дефолтный синхронный бэкенд; ядро работает без потоков | [Джобы и многопоточность](./guides/jobs.md) |
| `jobs/JobHandle.hpp` | Opaque value-хэндл выполняемой работы | [Джобы и многопоточность](./guides/jobs.md) |
| `jobs/FunctionRef.hpp` | Невладеющая ссылка на callable без аллокаций для тел горячих циклов | [Джобы и многопоточность](./guides/jobs.md) |

### `include/ecs/registry` — возможности уровня реестра

| Заголовок | Назначение | Руководство |
|-----------|------------|-------------|
| `registry/Commands.hpp` | Типы отложенных команд (`CreateEntityCmd`, `AddComponentsCmd`, `CookCmd`, …) | [Команды](./guides/commands.md) |
| `registry/Events.hpp` | Полезные нагрузки `PrefabEvt` / `CreatedEntityEvt` | [Рецепты и «готовка»](./guides/recipes-and-cooking.md) |
| `registry/CommandQueue.hpp` | Очередь команд на кадр с токен-доступом | [Команды](./guides/commands.md) |
| `registry/RegistryRegistrator.hpp` | Связывает имя реестра с его системами (`Registry::Create`) | [Системы и планирование](./guides/systems-and-scheduling.md) |

### `include/event` — обобщённая система событий

| Заголовок | Назначение | Руководство |
|-----------|------------|-------------|
| `event/EventSystem.hpp` | Обобщённый диспетчер по ключам с приоритетом | [События](./guides/events.md) |
| `event/Listener.hpp` · `event/AbstractListener.hpp` | Типизированные слушатели | [События](./guides/events.md) |
| `event/Event.hpp` · `event/AbstractEvent.hpp` | База полезной нагрузки события | [События](./guides/events.md) |

### `include/collections` — переиспользуемые контейнеры

| Заголовок | Назначение | Руководство |
|-----------|------------|-------------|
| `collections/Context.hpp` | Хранилище синглтонов по типу (`ctx()`) | [Быстрый старт](./guides/getting-started.md) |
| `collections/CommandQueue.hpp` | Обобщённая очередь отложенных вызовов | [Команды](./guides/commands.md) |
| `collections/DirectedAcyclicGraph.hpp` | DAG за планированием систем | [Системы и планирование](./guides/systems-and-scheduling.md) |
| `collections/BitSet.hpp` | Битсет фиксированной ширины для архетипов | — |

### `include/reg` и `include/common_recs`

| Заголовок | Назначение |
|-----------|------------|
| `reg/Registrator.hpp` | RAII-регистрация имён/типов со стратегиями `DEFAULT` / `UNIQUE` |
| `common_recs/utils/BaseError.hpp` | Базовые типы именованных ошибок |
| `common_recs/utils/ClassUtils.hpp` | Хелперы имён типов / хеширования |

---

<sub>Справочник API в `api/` генерируется — см.
[сборку документации](../README.md#building-the-api-reference).</sub>
