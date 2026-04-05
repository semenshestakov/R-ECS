#include "../../include/ecs/systems/SystemRegistrator.hpp"
#include "ComponentsClass.hpp"
#include "SystemsClass.hpp"
#include "ecs/Registry.hpp"
#include "gtest/gtest.h"

using namespace ecs;


TEST(AutoRegistrationTest, RegistryRegistratorSize)
{
    EXPECT_EQ(SystemRegistrator::Registrator::size(), 4);
}


TEST(AutoRegistrationTest, SystemsIsRegistered)
{
    EXPECT_EQ(SystemTestUpdate::IsRegistered, false);
    EXPECT_EQ(SystemTestId::IsRegistered, false);
    EXPECT_EQ(AutoSystem1::IsRegistered, true);
}


TEST(AutoRegistrationTest, SystemsCount)
{
    {
        SystemsManager* systemsManager = SystemRegistrator::GetSystemsManager("test1");
        EXPECT_EQ(systemsManager->size(), 2);
    }

    {
        SystemsManager* systemsManager = SystemRegistrator::GetSystemsManager("test2");
        EXPECT_EQ(systemsManager->size(), 1);
    }
}

TEST(AutoRegistrationTest, SimpleSystems)
{

    auto registry = Registry(*SystemRegistrator::GetSystemsManager("test1"));
    registry.Init();

    EXPECT_EQ(AutoSystem1::IsUpdated, false);
    EXPECT_EQ(AutoSystem2::IsUpdated, false);

    registry.Update();
    EXPECT_EQ(AutoSystem1::IsUpdated, true);
    EXPECT_EQ(AutoSystem2::IsUpdated, true);
}
