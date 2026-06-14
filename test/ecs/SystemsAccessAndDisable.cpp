#include "gtest/gtest.h"
#include <algorithm>
#include <string>
#include <vector>
#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"


using namespace ecs;


namespace
{
    std::vector<std::string> g_order;

    int indexOf(const std::string& name)
    {
        const auto it = std::ranges::find(g_order, name);
        return it == g_order.end() ? -1 : static_cast<int>(std::distance(g_order.begin(), it));
    }

    struct ComponentA {};
    struct ComponentB {};

    struct WriterSystem final : ISystem<WriterSystem>
    {
        ECS_REGISTRY("test_access")
        ECS_ACCESS(ecs::Write<ComponentA>)
        void Update(Registry&, const UpdateState&) override { g_order.emplace_back("Writer"); }
    };

    struct ReaderSystem final : ISystem<ReaderSystem>
    {
        ECS_REGISTRY("test_access")
        ECS_ACCESS(ecs::Read<ComponentA>, ecs::Read<ComponentB>)
        void Update(Registry&, const UpdateState&) override { g_order.emplace_back("Reader"); }
    };

    struct BaseSystem final : ISystem<BaseSystem>
    {
        ECS_REGISTRY("test_access")
        void Update(Registry&, const UpdateState&) override { g_order.emplace_back("Base"); }
    };

    struct MidSystem final : ISystem<MidSystem>
    {
        ECS_REGISTRY("test_access")
        ECS_DEPENDENT_SYSTEMS(BaseSystem)
        void Update(Registry&, const UpdateState&) override { g_order.emplace_back("Mid"); }
    };

    struct TopSystem final : ISystem<TopSystem>
    {
        ECS_REGISTRY("test_access")
        ECS_DEPENDENT_SYSTEMS(MidSystem)
        void Update(Registry&, const UpdateState&) override { g_order.emplace_back("Top"); }
    };

    struct WeakSystem final : ISystem<WeakSystem>
    {
        ECS_REGISTRY("test_access")
        ECS_WEAK_DEPENDENT_SYSTEMS(BaseSystem)
        void Update(Registry&, const UpdateState&) override { g_order.emplace_back("Weak"); }
    };
}


class SystemsAccessAndDisableTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_order.clear();
        registry = Registry::Create("test_access");
        registry.Init();
    }

    Registry registry = Registry();
};


TEST_F(SystemsAccessAndDisableTest, ReaderRunsAfterWriter)
{
    registry.Update();

    ASSERT_NE(indexOf("Writer"), -1);
    ASSERT_NE(indexOf("Reader"), -1);
    EXPECT_LT(indexOf("Writer"), indexOf("Reader"));
}


TEST_F(SystemsAccessAndDisableTest, WeakDependencyStillOrdersAfterTarget)
{
    registry.Update();

    ASSERT_NE(indexOf("Base"), -1);
    ASSERT_NE(indexOf("Weak"), -1);
    EXPECT_LT(indexOf("Base"), indexOf("Weak"));
}


TEST_F(SystemsAccessAndDisableTest, AllSystemsRunWhenNothingDisabled)
{
    registry.Update();

    for (const char* name : {"Writer", "Reader", "Base", "Mid", "Top", "Weak"})
        EXPECT_NE(indexOf(name), -1) << name << " did not run";
}


TEST_F(SystemsAccessAndDisableTest, DisablePropagatesThroughHardDependents)
{
    registry.Systems().Disable<BaseSystem>();
    registry.Update();

    EXPECT_EQ(indexOf("Base"), -1);
    EXPECT_EQ(indexOf("Mid"), -1);
    EXPECT_EQ(indexOf("Top"), -1);

    EXPECT_NE(indexOf("Weak"), -1);
    EXPECT_NE(indexOf("Writer"), -1);
    EXPECT_NE(indexOf("Reader"), -1);
}


TEST_F(SystemsAccessAndDisableTest, DisableDoesNotPropagateThroughDataEdges)
{
    registry.Systems().Disable<WriterSystem>();
    registry.Update();

    EXPECT_EQ(indexOf("Writer"), -1);
    EXPECT_NE(indexOf("Reader"), -1);
}


TEST_F(SystemsAccessAndDisableTest, DisableLeafOnlyKeepsDependencies)
{
    registry.Systems().Disable<TopSystem>();
    registry.Update();

    EXPECT_EQ(indexOf("Top"), -1);
    EXPECT_NE(indexOf("Mid"), -1);
    EXPECT_NE(indexOf("Base"), -1);
}


TEST_F(SystemsAccessAndDisableTest, EnableRestoresCascade)
{
    registry.Systems().Disable<BaseSystem>();
    registry.Systems().Enable<BaseSystem>();
    registry.Update();

    for (const char* name : {"Base", "Mid", "Top"})
        EXPECT_NE(indexOf(name), -1) << name << " did not run after enable";
}


TEST_F(SystemsAccessAndDisableTest, IsEnabledReflectsCascade)
{
    EXPECT_TRUE(registry.Systems().IsEnabled<BaseSystem>());
    EXPECT_TRUE(registry.Systems().IsEnabled<MidSystem>());

    registry.Systems().Disable<BaseSystem>();

    EXPECT_FALSE(registry.Systems().IsEnabled<BaseSystem>());
    EXPECT_FALSE(registry.Systems().IsEnabled<MidSystem>());
    EXPECT_FALSE(registry.Systems().IsEnabled<TopSystem>());
    EXPECT_TRUE(registry.Systems().IsEnabled<WeakSystem>());
    EXPECT_TRUE(registry.Systems().IsEnabled<ReaderSystem>());
}
