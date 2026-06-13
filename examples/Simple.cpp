#include <cstddef>

#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/entities/PrefabEntity.hpp"


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
    auto registry = ecs::Registry::Create("MyName");
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
