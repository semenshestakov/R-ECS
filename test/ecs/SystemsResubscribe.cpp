#include "gtest/gtest.h"
#include <vector>
#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"


using namespace ecs;


namespace
{
    std::vector<systemHash_t> g_resubOrder;

    struct ResubEvt {};

    struct ResubBaseSystem final : ISystem<ResubBaseSystem>
    {
        void OnResub(Registry&, const ResubEvt&)
        {
            g_resubOrder.push_back(getSystemHash<SelfSystemCls>());
        }
        ECS_EVENT(OnResub, ResubEvt)
    };

    struct ResubDepSystem final : ISystem<ResubDepSystem>
    {
        ECS_DEPENDENT_SYSTEMS(ResubBaseSystem)

        void OnResub(Registry&, const ResubEvt&)
        {
            g_resubOrder.push_back(getSystemHash<SelfSystemCls>());
        }
        ECS_EVENT(OnResub, ResubEvt)
    };
}


class SystemsResubscribeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_resubOrder.clear();

        registry = Registry();
        registry.Systems().Register<ResubBaseSystem>();
        registry.Init();
    }

    Registry registry = Registry();
};


TEST_F(SystemsResubscribeTest, SystemAddedAfterInitIsNotSubscribedUntilUpdate)
{
    registry.Systems().Register<ResubDepSystem>(); // schedule becomes dirty

    g_resubOrder.clear();
    registry.Events().OnEvent<ResubEvt>({});

    // Re-subscription is deferred to the next Update, so only the base handler fires.
    EXPECT_EQ(g_resubOrder.size(), 1u);
    EXPECT_EQ(g_resubOrder.front(), getSystemHash<ResubBaseSystem>());
}


TEST_F(SystemsResubscribeTest, UpdateResubscribesAfterScheduleRecompute)
{
    registry.Systems().Register<ResubDepSystem>();

    EXPECT_TRUE(registry.Systems().isScheduleDirty());
    registry.Update();                       // rebuild schedule + refresh subscriptions
    EXPECT_FALSE(registry.Systems().isScheduleDirty());

    g_resubOrder.clear();
    registry.Events().OnEvent<ResubEvt>({});

    ASSERT_EQ(g_resubOrder.size(), 2u);
    // Order follows the recomputed schedule: base before its dependent.
    EXPECT_EQ(g_resubOrder[0], getSystemHash<ResubBaseSystem>());
    EXPECT_EQ(g_resubOrder[1], getSystemHash<ResubDepSystem>());
}


TEST_F(SystemsResubscribeTest, RepeatedEventsDoNotDuplicateHandlers)
{
    registry.Systems().Register<ResubDepSystem>();
    registry.Update();

    g_resubOrder.clear();
    registry.Events().OnEvent<ResubEvt>({});
    const std::size_t firstCount = g_resubOrder.size();

    g_resubOrder.clear();
    registry.Events().OnEvent<ResubEvt>({});

    EXPECT_EQ(g_resubOrder.size(), firstCount); // no duplicate subscriptions accumulate
    EXPECT_EQ(firstCount, 2u);
}
