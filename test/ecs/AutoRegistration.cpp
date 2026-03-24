#include "gtest/gtest.h"
#include "ecs/Registry.hpp"
#include "ecs/registry/RegistryRegistrator.hpp"
#include "ecs/entities/UMapEntitiesManager.hpp"
#include "ComponentsClass.hpp"
#include "SystemsClass.hpp"

using namespace ecs;


TEST(AutoRegistrationTest, RegistryRegistratorSize)
{
    EXPECT_EQ(RegistryRegistrator::Registrator::size(), 4);
}

TEST(AutoRegistrationTest, ComponentsIsRegistered)
{
    EXPECT_EQ(PositionX::IsRegistered, true);
    EXPECT_EQ(PositionY::IsRegistered, true);
    EXPECT_EQ(PositionZ::IsRegistered, false);
}


TEST(AutoRegistrationTest, TestUnknownRegistryName)
{
    EXPECT_EQ(RegistryRegistrator::GetComponentsManager("test_null"), nullptr);
}


TEST(AutoRegistrationTest, SimpleComponents)
{
    ComponentsManager* componentsManager = RegistryRegistrator::GetComponentsManager("test1");
    EXPECT_NE(componentsManager, nullptr);
    {
        ComponentsPtr components = componentsManager->CreateComponents();
        components->initialize();

        EXPECT_EQ(components->get<PositionX>()->x, PositionX().x);
        EXPECT_EQ(components->get<PositionY>()->y, PositionY().y);

        EXPECT_EQ(components->get<PositionZ>(), nullptr);
    }
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
        SystemsManager* systemsManager = RegistryRegistrator::GetSystemsManager("test1");
        EXPECT_EQ(systemsManager->size(), 2);
    }

    {
        SystemsManager* systemsManager = RegistryRegistrator::GetSystemsManager("test2");
        EXPECT_EQ(systemsManager->size(), 1);
    }
}

TEST(AutoRegistrationTest, SimpleSystems)
{

    auto registry = Registry(
        EntitiesManager::Create<UMapEntitiesManager>(),
        *RegistryRegistrator::GetComponentsManager("test1"),
        *RegistryRegistrator::GetSystemsManager("test1")
        );
    registry.Init();

    EXPECT_EQ(AutoSystem1::IsUpdated, false);
    EXPECT_EQ(AutoSystem2::IsUpdated, false);

    registry.Update();
    EXPECT_EQ(AutoSystem1::IsUpdated, true);
    EXPECT_EQ(AutoSystem2::IsUpdated, true);
}
