# Replication Entities Components Systems (R-ECS)

---
## ⚡Description
This is a modern, high-performance Entity-Component-System framework written in 20, designed for game development and 
real-time simulation applications. The framework implements the archetype-based ECS architecture, which provides excellent 
cache locality and runtime performance while maintaining a clean, intuitive API.

---
## 🎯 Roadmap / Feature
| Roadmap / Feature                                                                                              |  Version  | Tests |
|:---------------------------------------------------------------------------------------------------------------|:---------:|:-----:|
| DynamicComponentsFactory (DEPRICATED)                                                                          | v0.1.0 ⚠️ |  ⚠️   |
| ComponentsFactory (DEPRICATED)                                                                                 | v0.2.0 ⚠️ |  ⚠️   |
| Registry, auto-registration, iterators                                                                         | v0.3.0 ✅  |   ✅   |
| Event system                                                                                                   | v0.4.0 ✅  |   ✅   |
| EventSystem for ECS                                                                                            | v0.5.0 ✅  |   ✅   |
| ECS context                                                                                                    | v0.6.0 ✅  |   ✅   |
| Directed Acyclic Graph, Systems Schedule                                                                       | v0.7.0 ✅  |   ✅   |
| Refactor: Entity / EntitiesManager <br/> New: PrefabEntity, EntityWrapper, Archetype, EntitiesArchetypeStorage | v0.8.0 ✅  |   ✅   |
| Cooking: Cooker, Recipe (hierarchy PrefabEntity)                                                               | v0.9.0 ❌  |   ❌   |


## 🚀 Quick Start

```c++
#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/entities/PrefabEntity.hpp"
#include "ecs/systems/SystemRegistrator.hpp"


struct Vel2d
{
    float x, y;
};

struct Position2d
{
    float x, y;
};


struct MoveSystem final : ecs::ISystem<MoveSystem>
{
    ECS_REGISTRY("MyName")

    void Update(ecs::Registry& registry, const ecs::UpdateState& state) override
    {
        for (auto [pos2d, vel2d] : registry.Entities().view<Position2d, Vel2d>())
        {
            pos2d.x += vel2d.x;
            pos2d.y += vel2d.y;

            vel2d.x *= 0.98f;
            vel2d.y *= 0.98f;
        }
    }

};


int main()
{
    auto registry = ecs::Registry(
        *ecs::SystemRegistrator::GetSystemsManager("MyName")
        );
    registry.Init();

    ecs::PrefabEntity prefab;
    prefab.AddComponent<Position2d>(0.0f, 0.0f);
    prefab.AddComponent<Vel2d>(100.0f, 100.0f);

    for (std::size_t i = 0; i < 100; i++)
    {
        ecs::EntityWrapper entity = registry.Entities().Create(prefab);

        entity.GetComponent<Position2d>().x = static_cast<float>(i);
        entity.GetComponent<Position2d>().y = -static_cast<float>(i);
    }

    registry.Update();
}
```
