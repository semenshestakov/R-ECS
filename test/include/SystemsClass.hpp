#pragma once
#include <gmock/gmock.h>
#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ComponentsClass.hpp"


// ================================ Simple Systems ================================

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
        for (auto [testId] : registry.Entities().view<TestId>())
        {
            testId.id++;
        }
        static const unsigned int s_Counter2 = ++Counter;

        for (auto [testId, position] : registry.Entities().view<TestId, PositionY>())
        {
            testId.id++;
        }
        static const unsigned int s_Counter3 = ++Counter;

        for (auto [testId, position] : registry.Entities().view<TestId, PositionZ>()) // filtered all
        {
            testId.id++;
        }
    }
};

// ================================ Auto Reg Systems ================================

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

// ================================ Event Systems ================================

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


// ================================ Schedule Systems ================================

inline std::vector<std::string> g_systemCallOrder;


struct ResourceSystem final : ecs::ISystem<ResourceSystem>
{
    ECS_REGISTRY("test_schedule")

    void Update(ecs::Registry & registry, const ecs::UpdateState & state) override
    {
        g_systemCallOrder.emplace_back("ResourceSystem");
    }
};


struct InputSystem final : ecs::ISystem<InputSystem>
{
    ECS_REGISTRY("test_schedule")

    void Update(ecs::Registry & registry, const ecs::UpdateState & state) override
    {
        g_systemCallOrder.emplace_back("InputSystem");
    }
};


struct PhysicsSystem final : ecs::ISystem<PhysicsSystem>
{
    ECS_REGISTRY("test_schedule")

    void Update(ecs::Registry & registry, const ecs::UpdateState & state) override
    {
        g_systemCallOrder.emplace_back("PhysicsSystem");
    }
};


struct RenderSystem final : ecs::ISystem<RenderSystem>
{
    ECS_REGISTRY("test_schedule")
    ECS_DEPENDENT_SYSTEMS(ResourceSystem, InputSystem, PhysicsSystem)

    void Update(ecs::Registry & registry, const ecs::UpdateState & state) override
    {
        g_systemCallOrder.emplace_back("RenderSystem");
    }
};


struct AISystem final : ecs::ISystem<AISystem>
{
    ECS_REGISTRY("test_schedule")

    void Update(ecs::Registry & registry, const ecs::UpdateState & state) override
    {
        g_systemCallOrder.emplace_back("AISystem");
    }
};
