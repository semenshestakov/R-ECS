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

        registry = Registry(*SystemRegistrator::GetSystemsManager("test_schedule"));
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
