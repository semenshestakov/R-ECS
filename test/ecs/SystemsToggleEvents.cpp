#include "gtest/gtest.h"
#include <algorithm>
#include <vector>
#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/registry/Commands.hpp"


using namespace ecs;


namespace
{
    std::vector<systemHash_t> g_toggleCalls;

    struct ToggleEvt {};

    struct ToggleBaseSystem final : ISystem<ToggleBaseSystem>
    {
        void OnToggle(Registry&, const ToggleEvt&)
        {
            g_toggleCalls.push_back(getSystemHash<SelfSystemCls>());
        }
        ECS_EVENT(OnToggle, ToggleEvt)
    };

    struct ToggleDepSystem final : ISystem<ToggleDepSystem>
    {
        ECS_DEPENDENT_SYSTEMS(ToggleBaseSystem)

        void OnToggle(Registry&, const ToggleEvt&)
        {
            g_toggleCalls.push_back(getSystemHash<SelfSystemCls>());
        }
        ECS_EVENT(OnToggle, ToggleEvt)
    };

    struct ToggleIndependentSystem final : ISystem<ToggleIndependentSystem>
    {
        void OnToggle(Registry&, const ToggleEvt&)
        {
            g_toggleCalls.push_back(getSystemHash<SelfSystemCls>());
        }
        ECS_EVENT(OnToggle, ToggleEvt)
    };

    bool fired(const systemHash_t hash)
    {
        return std::ranges::find(g_toggleCalls, hash) != g_toggleCalls.end();
    }
}


class SystemsToggleEventsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_toggleCalls.clear();

        registry = Registry();
        registry.Systems().Register<ToggleBaseSystem>();
        registry.Systems().Register<ToggleDepSystem>();
        registry.Systems().Register<ToggleIndependentSystem>();
        registry.Init();   // subscribes all enabled systems
    }

    Registry registry = Registry();

    static systemHash_t base()        { return getSystemHash<ToggleBaseSystem>(); }
    static systemHash_t dep()         { return getSystemHash<ToggleDepSystem>(); }
    static systemHash_t independent() { return getSystemHash<ToggleIndependentSystem>(); }
};


TEST_F(SystemsToggleEventsTest, AllSubscribedReceiveEventsByDefault)
{
    registry.Events().OnEvent<ToggleEvt>({});

    EXPECT_TRUE(fired(base()));
    EXPECT_TRUE(fired(dep()));
    EXPECT_TRUE(fired(independent()));
}


TEST_F(SystemsToggleEventsTest, DisabledSystemStopsReceivingEventsAfterUpdate)
{
    registry.Systems().Disable<ToggleBaseSystem>();
    registry.Update();   // reconciles subscriptions

    registry.Events().OnEvent<ToggleEvt>({});

    EXPECT_FALSE(fired(base()));
    EXPECT_TRUE(fired(independent()));
}


TEST_F(SystemsToggleEventsTest, DisableCascadesUnsubscribeToHardDependents)
{
    registry.Systems().Disable<ToggleBaseSystem>();
    registry.Update();

    registry.Events().OnEvent<ToggleEvt>({});

    EXPECT_FALSE(fired(base()));
    EXPECT_FALSE(fired(dep()));        // hard-dependent: unsubscribed by cascade
    EXPECT_TRUE(fired(independent())); // unrelated: keeps receiving events
}


TEST_F(SystemsToggleEventsTest, ReconciliationIsDeferredUntilNextUpdate)
{
    registry.Systems().Disable<ToggleBaseSystem>();
    // No Update yet: the handler is still attached, so it still fires this frame.
    registry.Events().OnEvent<ToggleEvt>({});

    EXPECT_TRUE(fired(base()));
}


TEST_F(SystemsToggleEventsTest, EnableResubscribesHandlers)
{
    registry.Systems().Disable<ToggleBaseSystem>();
    registry.Update();

    registry.Systems().Enable<ToggleBaseSystem>();
    registry.Update();

    registry.Events().OnEvent<ToggleEvt>({});

    EXPECT_TRUE(fired(base()));
    EXPECT_TRUE(fired(dep()));
    EXPECT_TRUE(fired(independent()));
}


TEST_F(SystemsToggleEventsTest, ResubscribeDoesNotDuplicateHandlers)
{
    registry.Systems().Disable<ToggleBaseSystem>();
    registry.Update();
    registry.Systems().Enable<ToggleBaseSystem>();
    registry.Update();

    registry.Events().OnEvent<ToggleEvt>({});
    const std::size_t once = g_toggleCalls.size();

    g_toggleCalls.clear();
    registry.Events().OnEvent<ToggleEvt>({});

    EXPECT_EQ(g_toggleCalls.size(), once); // no accumulation of stale subscriptions
}


TEST_F(SystemsToggleEventsTest, DisableSystemCmdUnsubscribesViaQueue)
{
    registry.Commands().Push(DisableSystemCmd<ToggleBaseSystem>{});
    registry.Update();   // flush applies the disable, then subscriptions reconcile

    EXPECT_FALSE(registry.Systems().IsEnabled<ToggleBaseSystem>());

    registry.Events().OnEvent<ToggleEvt>({});

    EXPECT_FALSE(fired(base()));
    EXPECT_FALSE(fired(dep()));
    EXPECT_TRUE(fired(independent()));
}


TEST_F(SystemsToggleEventsTest, EnableSystemCmdResubscribesViaQueue)
{
    registry.Commands().Push(DisableSystemCmd<ToggleBaseSystem>{});
    registry.Update();

    registry.Commands().Push(EnableSystemCmd<ToggleBaseSystem>{});
    registry.Update();

    EXPECT_TRUE(registry.Systems().IsEnabled<ToggleBaseSystem>());

    registry.Events().OnEvent<ToggleEvt>({});

    EXPECT_TRUE(fired(base()));
    EXPECT_TRUE(fired(dep()));
}
