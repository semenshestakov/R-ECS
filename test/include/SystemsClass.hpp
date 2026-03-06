#pragma once
#include <gmock/gmock.h>
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


struct Event1
{
    int value = 1;
};

struct Event2
{
    int value = 2;
};



struct SystemEventHandler1 final : ecs::ISystem<SystemEventHandler1>
{
    ECS_REGISTRY("test_event")

    void OnEvent1(ecs::Registry& registry, const Event1& event)
    {
        callInt(event.value);
    }
    ECS_EVENT(OnEvent1, Event1)

    MOCK_METHOD(void, callInt, (int));
};


struct SystemEventHandler2 final : ecs::ISystem<SystemEventHandler2>
{
    ECS_REGISTRY("test_event")

    void OnEvent1(ecs::Registry& registry, const Event1& event)
    {
        callInt(event.value);
    }
    ECS_EVENT(OnEvent1, Event1)

    void OnEvent2(ecs::Registry& registry, const Event2& event)
    {
        callInt(event.value);
    }
    ECS_EVENT(OnEvent2, Event2)


    MOCK_METHOD(void, callInt, (int));
};
