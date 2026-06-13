// ───────────────────────────────────────────────────────────────────────
//  R-ECS example — systems & DAG scheduling
//
//  Five systems wired with ECS_DEPENDENT_SYSTEMS. The registry runs them in
//  topological order:
//
//      Resource ┐
//      Input    ├─▶ Render ─▶ PostRender
//      Physics  ┘
//
//  See: docs/en/guides/systems-and-scheduling.md
// ───────────────────────────────────────────────────────────────────────
#include <iostream>

#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"

struct ResourceSystem final : ecs::ISystem<ResourceSystem>
{
    ECS_REGISTRY("game")
    void Update(ecs::Registry&, const ecs::UpdateState&) override
    { std::cout << "Resource "; }
};

struct InputSystem final : ecs::ISystem<InputSystem>
{
    ECS_REGISTRY("game")
    void Update(ecs::Registry&, const ecs::UpdateState&) override
    { std::cout << "Input "; }
};

struct PhysicsSystem final : ecs::ISystem<PhysicsSystem>
{
    ECS_REGISTRY("game")
    void Update(ecs::Registry&, const ecs::UpdateState&) override
    { std::cout << "Physics "; }
};

struct RenderSystem final : ecs::ISystem<RenderSystem>
{
    ECS_REGISTRY("game")
    ECS_DEPENDENT_SYSTEMS(ResourceSystem, InputSystem, PhysicsSystem)
    void Update(ecs::Registry&, const ecs::UpdateState&) override
    { std::cout << "-> Render "; }
};

struct PostRenderSystem final : ecs::ISystem<PostRenderSystem>
{
    ECS_REGISTRY("game")
    ECS_DEPENDENT_SYSTEMS(RenderSystem)
    void Update(ecs::Registry&, const ecs::UpdateState&) override
    { std::cout << "-> PostRender\n"; }
};

int main()
{
    ecs::Registry registry = ecs::Registry::Create("game");
    registry.Init();

    // Render always prints after Resource/Input/Physics; PostRender last.
    registry.Update();
}
