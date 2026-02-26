#include "ecs/Registry.hpp"
#include "../include/ComponentsClass.hpp"
#include "ecs/entities/UMapEntitiesManager.hpp"
#include "ecs/utils/RegistryError.hpp"
#include "gtest/gtest.h"


using namespace ecs;

using RegistryUMap = Registry;


TEST(RegistryUMapTest, CreateWithGeneratedId)
{
    ComponentsManager componentsManager;
    componentsManager.Register<Position2d>();
    componentsManager.Register<Position3d>();

    auto registry = Registry(EntitiesManager::Create<UMapEntitiesManager>(), componentsManager);

    {
        auto& components = registry.Create();
        EXPECT_NE(registry.get(1), nullptr);
    }

    {
        auto& components = registry.Create();
        EXPECT_NE(registry.get(2), nullptr);
    }

    EXPECT_NE(registry.get(0), registry.get(1));

    {
        const auto& components = registry.mustGet(1);
        EXPECT_EQ(components.mustGet<Position3d>().x, Position3d().x);

        components.mustGet<Position3d>().x += 5.0;
        EXPECT_EQ(components.mustGet<Position3d>().x, Position3d().x + 5.0);
    }
}


TEST(RegistryUMapTest, CreateWithId)
{
    ComponentsManager componentsManager;
    componentsManager.Register<Position2d>();
    componentsManager.Register<Position3d>();

    auto registry = Registry(EntitiesManager::Create<UMapEntitiesManager>(), componentsManager);
    {
        auto& components = registry.Create(100);
        EXPECT_NE(registry.get(100), nullptr);
    }

    {
        auto& components = registry.Create(200);
        EXPECT_NE(registry.get(200), nullptr);
    }

    EXPECT_THROW(
        {
            auto& components = registry.Create(100);
        },
        error::InvalidEntityId);
}
