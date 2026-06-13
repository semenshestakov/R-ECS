#include "ecs/systems/SystemRegistrator.hpp"
#include "SystemsClass.hpp"
#include "ecs/Registry.hpp"
#include "gtest/gtest.h"


using namespace ecs;


TEST(AutoRegistrationTest, SystemsCount)
{
    EXPECT_EQ(RegistryRegistrator::Get("test1").systemRegIndexes.size(), 2);
    EXPECT_EQ(RegistryRegistrator::Get("test2").systemRegIndexes.size(), 1);
}

TEST(AutoRegistrationTest, SimpleSystems)
{

    const SystemsManager systemManager = SystemsManager::Create(RegistryRegistrator::Get("test1").systemRegIndexes);
    auto registry = Registry(systemManager); // copy
    registry.Init();

    EXPECT_EQ(AutoSystem1::IsUpdated, false);
    EXPECT_EQ(AutoSystem2::IsUpdated, false);

    registry.Update();
    EXPECT_EQ(AutoSystem1::IsUpdated, true);
    EXPECT_EQ(AutoSystem2::IsUpdated, true);
}


TEST(AutoRegistrationTest, MultipleUpdates_SystemsCalledEachTime)
{
    AutoSystem1::IsUpdated = false;
    AutoSystem2::IsUpdated = false;

    const SystemsManager systemManager = SystemsManager::Create(RegistryRegistrator::Get("test1").systemRegIndexes);
    auto registry = Registry(systemManager);
    registry.Init();

    registry.Update();
    EXPECT_TRUE(AutoSystem1::IsUpdated);
    EXPECT_TRUE(AutoSystem2::IsUpdated);

    AutoSystem1::IsUpdated = false;
    AutoSystem2::IsUpdated = false;

    registry.Update();
    EXPECT_TRUE(AutoSystem1::IsUpdated);
    EXPECT_TRUE(AutoSystem2::IsUpdated);
}


TEST(AutoRegistrationTest, test2_ContainsOnlyAutoSystem2)
{
    // AutoSystem2 declares ECS_REGISTRY("test1", "test2"), AutoSystem1 only "test1"
    const auto mgr = SystemsManager::Create(RegistryRegistrator::Get("test2").systemRegIndexes);
    EXPECT_EQ(mgr.size(), 1);
    EXPECT_NE(mgr.TryGet<AutoSystem2>(), nullptr);
    EXPECT_EQ(mgr.TryGet<AutoSystem1>(), nullptr);
}


TEST(AutoRegistrationTest, TryGet_ExistingSystem)
{
    const auto mgr = SystemsManager::Create(RegistryRegistrator::Get("test1").systemRegIndexes);
    EXPECT_NE(mgr.TryGet<AutoSystem1>(), nullptr);
    EXPECT_NE(mgr.TryGet<AutoSystem2>(), nullptr);
}


TEST(AutoRegistrationTest, TryGet_MissingSystem_ReturnsNull)
{
    const auto mgr = SystemsManager::Create(RegistryRegistrator::Get("test1").systemRegIndexes);
    EXPECT_EQ(mgr.TryGet<SystemTestUpdate>(), nullptr);
}
