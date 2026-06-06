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
