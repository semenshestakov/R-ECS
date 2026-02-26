#include "../include/ComponentsClass.hpp"
#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/entities/UMapEntitiesManager.hpp"
#include "ecs/entities/ranges/EntitiesViews.hpp"
#include "ecs/systems/SystemsManager.hpp"
#include "gtest/gtest.h"
#include "SystemsClass.hpp"

using namespace ecs;


TEST(Systems, CreateSystems)
{
    Registry registry;
    {
        ComponentsManager* componentsManager = RegistryRegistrator::GetComponentsManager("test1");  // PositionX, PositionY, TestId
        SystemsManager systemManger;
        systemManger.Register<SystemTestUpdate>();
        systemManger.Register<SystemTestId>();

        registry =  Registry(EntitiesManager::Create<UMapEntitiesManager>(), *componentsManager, systemManger);
    }
    EXPECT_EQ(registry.m_systemManager.size(), 2);
}


TEST(Systems, SimpleUpdateSystems)
{
    Registry registry;
    {
        ComponentsManager* componentsManager = RegistryRegistrator::GetComponentsManager("test1");  // PositionX, PositionY, TestId
        SystemsManager systemManger;
        systemManger.Register<SystemTestUpdate>();
        systemManger.Register<SystemTestId>();

        registry =  Registry(EntitiesManager::Create<UMapEntitiesManager>(), *componentsManager, systemManger);
        registry.Init();
    }

    for (std::size_t i = 1; i < 100; i++)
    {
        registry.Update();
        EXPECT_EQ(SystemTestUpdate::Counter, i);
    }
}


TEST(Systems, TestIteratorsWithEntities)
{
    Registry registry;
    {
        ComponentsManager* componentsManager = RegistryRegistrator::GetComponentsManager("test1");  // PositionX, PositionY, TestId
        SystemsManager systemManger;
        systemManger.Register<SystemTestUpdate>();
        systemManger.Register<SystemTestId>();

        registry =  Registry(EntitiesManager::Create<UMapEntitiesManager>(), *componentsManager, systemManger);
        registry.Init();
    }
    EXPECT_EQ(SystemTestId::Counter, 3);

    constexpr std::size_t MAX_ENTITIES = 20;
    for (std::size_t i = 0; i < MAX_ENTITIES; i++)
    {
        Components& components = registry.Create(i);
        EXPECT_EQ(components.mustGet<TestId>().id, TestId().id);
        registry.Update();
        EXPECT_EQ(components.mustGet<TestId>().id, TestId().id + SystemTestId::Counter);
    }

    for (std::size_t i = 0; i < MAX_ENTITIES; i++)
    {
        Components& components = registry.mustGet(i);
        EXPECT_EQ(components.mustGet<TestId>().id, TestId().id + (MAX_ENTITIES - i) * SystemTestId::Counter);
    }
}