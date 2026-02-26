#pragma once
#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/registry/AutoRegistry.hpp"
#include "ComponentsClass.hpp"



struct SystemTestUpdate final : ecs::ISystem<SystemTestUpdate>
{
    inline static unsigned int Counter = {0};

    void Update(ecs::Registry & registry, const ecs::UpdateState & state) override
    {
        Counter++;
    }
};


struct SystemTestId final : ecs::ISystem<SystemTestId>
{
    inline static unsigned int Counter = {0};
    void Update(ecs::Registry & registry, const ecs::UpdateState & state) override
    {
        for (auto& components : registry.view())
        {
            components.mustGet<TestId>().id++;
        }
        static const unsigned int s_Counter1 = ++Counter;

        for (auto [testId] : registry.view<TestId>())
        {
            testId.id++;
        }
        static const unsigned int s_Counter2 = ++Counter;

        for (auto [testId, position] : registry.view<TestId, PositionY>())
        {
            testId.id++;
        }
        static const unsigned int s_Counter3 = ++Counter;

        for (auto [testId, position] : registry.view<TestId, PositionZ>()) // filtered all
        {
            testId.id++;
        }
    }
};

struct AutoSystem1 final : ecs::ISystem<AutoSystem1>
{
    ECS_REGISTRY("test1")

    static inline bool IsUpdated = false;
    void Update(ecs::Registry & registry, const ecs::UpdateState & state) override { IsUpdated = true;}
};

struct AutoSystem2 final : ecs::ISystem<AutoSystem2>
{
    ECS_REGISTRY("test1", "test2")

    static inline bool IsUpdated = false;
    void Update(ecs::Registry & registry, const ecs::UpdateState & state) override { IsUpdated = true; }
};