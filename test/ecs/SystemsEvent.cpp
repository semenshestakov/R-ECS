#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/systems/SystemsManager.hpp"
#include "gtest/gtest.h"
#include "SystemsClass.hpp"


using namespace ecs;

class OrderSystemEventTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_systemEventCallOrder.clear();

        registry = Registry::Create("test_schedule");
        registry.Init();
    }

    Registry registry = Registry();
};


TEST_F(OrderSystemEventTest, EventCall_AllSubscribedSystemsCalled)
{
    ASSERT_EQ(g_systemEventCallOrder.size(), 0);
    registry.Events().OnEvent<EventCallOrder>({});
    ASSERT_EQ(g_systemEventCallOrder.size(), 5);
}

TEST_F(OrderSystemEventTest, EventCall_OrderRespectsDependencies)
{
    registry.Events().OnEvent<EventCallOrder>({});

    const auto resource = ecs::getSystemHash<ResourceSystem>();
    const auto input    = ecs::getSystemHash<InputSystem>();
    const auto physics  = ecs::getSystemHash<PhysicsSystem>();
    const auto render   = ecs::getSystemHash<RenderSystem>();
    const auto post     = ecs::getSystemHash<PostRenderSystem>();

    ASSERT_EQ(g_systemEventCallOrder.size(), 5);

    auto indexOf = [&](const ecs::systemHash_t hash)
    {
        return std::ranges::find(g_systemEventCallOrder, hash);
    };

    EXPECT_LT(indexOf(resource),    indexOf(render));
    EXPECT_LT(indexOf(input),       indexOf(render));
    EXPECT_LT(indexOf(physics),     indexOf(render));
    EXPECT_LT(indexOf(render),      indexOf(post));
}


TEST_F(OrderSystemEventTest, EventCall_SystemWithoutEventNotCalled)
{
    registry.Events().OnEvent<EventCallOrder>({});

    const auto ai = ecs::getSystemHash<AISystem>();
    const auto it = std::ranges::find(g_systemEventCallOrder, ai);

    EXPECT_EQ(it, g_systemEventCallOrder.end());
}


class EventSystemFlushTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_eventExecutionOrder.clear();

        registry = Registry::Create("test_ecs_event_system");
        registry.Init();
    }

    Registry registry;
};


TEST_F(EventSystemFlushTest, PushEvent_DoesNotTriggerImmediately)
{
    registry.Events().PushEvent(EventCallOrder{});

    EXPECT_TRUE(g_eventExecutionOrder.empty());
}


TEST_F(EventSystemFlushTest, PushEventTriggeredOnFlush)
{
    registry.Events().PushEvent(EventCallOrder{});

    registry.Update();

    EXPECT_EQ(g_eventExecutionOrder.size(), 3);
}


TEST_F(EventSystemFlushTest, EventCallOrderRespectsSystemDependencies)
{
    registry.Events().PushEvent(EventCallOrder{});
    registry.Update();

    ASSERT_EQ(g_eventExecutionOrder.size(), 3);

    const auto first  = ecs::getSystemHash<FirstEventSystem>();
    const auto second = ecs::getSystemHash<SecondEventSystem>();
    const auto third  = ecs::getSystemHash<ThirdEventSystem>();

    const auto itFirst  = std::ranges::find(g_eventExecutionOrder, first);
    const auto itSecond = std::ranges::find(g_eventExecutionOrder, second);
    const auto itThird  = std::ranges::find(g_eventExecutionOrder, third);

    ASSERT_NE(itFirst, g_eventExecutionOrder.end());
    ASSERT_NE(itSecond, g_eventExecutionOrder.end());
    ASSERT_NE(itThird, g_eventExecutionOrder.end());

    EXPECT_LT(itFirst, itThird);
    EXPECT_LT(itSecond, itThird);
}


TEST_F(EventSystemFlushTest, MultipleEventsPreserveSystemOrder)
{
    registry.Events().PushEvent(EventCallOrder{});
    registry.Events().PushEvent(EventCallOrder{});

    registry.Update();

    ASSERT_EQ(g_eventExecutionOrder.size(), 6);

    const auto third = ecs::getSystemHash<ThirdEventSystem>();

    EXPECT_EQ(g_eventExecutionOrder[2], third);
    EXPECT_EQ(g_eventExecutionOrder[5], third);
}


TEST_F(EventSystemFlushTest, DeterministicOrderBetweenFrames)
{
    registry.Events().PushEvent(EventCallOrder{});
    registry.Update();

    const auto firstRun = g_eventExecutionOrder;

    g_eventExecutionOrder.clear();

    registry.Events().PushEvent(EventCallOrder{});
    registry.Update();

    EXPECT_EQ(firstRun, g_eventExecutionOrder);
}


struct UnusedEvent { };

TEST_F(EventSystemFlushTest, EventWithoutSubscribersDoesNothing)
{
    registry.Events().PushEvent(UnusedEvent{});

    EXPECT_NO_THROW(registry.Update());
    EXPECT_TRUE(g_eventExecutionOrder.empty());
}


TEST_F(EventSystemFlushTest, IndependentSystemsExecuteBeforeDependent)
{
    registry.Events().PushEvent(EventCallOrder{});
    registry.Update();

    const auto first  = ecs::getSystemHash<FirstEventSystem>();
    const auto second = ecs::getSystemHash<SecondEventSystem>();
    const auto third  = ecs::getSystemHash<ThirdEventSystem>();

    const auto itFirst  = std::ranges::find(g_eventExecutionOrder, first);
    const auto itSecond = std::ranges::find(g_eventExecutionOrder, second);
    const auto itThird  = std::ranges::find(g_eventExecutionOrder, third);

    EXPECT_LT(itFirst, itThird);
    EXPECT_LT(itSecond, itThird);
}


TEST_F(EventSystemFlushTest, PushAfterFlushExecutesNextFrame)
{
    registry.Events().PushEvent(EventCallOrder{});
    registry.Update();

    const size_t firstFrameCalls = g_eventExecutionOrder.size();

    registry.Events().PushEvent(EventCallOrder{});
    registry.Update();

    EXPECT_EQ(g_eventExecutionOrder.size(), firstFrameCalls * 2);
}