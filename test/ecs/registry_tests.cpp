#include "ecs/entities/UMapEntitiesManager.hpp"
#include "components_class.hpp"
#include "ecs/Registry.hpp"
#include "ecs/utils/RegistryError.hpp"
#include "gtest/gtest.h"


using namespace ecs;

using RegistryUMap = Registry;


TEST(RegistryUMapTest, CreateWithGeneratedId)
{
    ComponentsManager factory;
    factory.Register<Position2d>();
    factory.Register<Position3d>();

    auto registry = Registry(EntitiesManager::Create<UMapEntitiesManager>(), factory);

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
    ComponentsManager factory;
    factory.Register<Position2d>();
    factory.Register<Position3d>();

    auto registry = Registry(EntitiesManager::Create<UMapEntitiesManager>(), factory);
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



TEST(Components, AutoRegistration)
{
#ifdef DEEP_TEST_ENABLE
    EXPECT_EQ(RegistryRegistrator::Registrator::size(), 2);
    EXPECT_EQ(PositionX::IsRegistered, true);
    EXPECT_EQ(PositionY::IsRegistered, true);
    EXPECT_EQ(PositionZ::IsRegistered, false);
#endif

    ComponentsManager* factory = RegistryRegistrator::GetComponentsManager("test1");
    EXPECT_NE(factory, nullptr);
    {
        ComponentsPtr components = factory->CreateComponents();
        components->initialize();

        EXPECT_EQ(components->get<PositionX>()->x, PositionX().x);
        EXPECT_EQ(components->get<PositionY>()->y, PositionY().y);

        EXPECT_EQ(components->get<PositionZ>(), nullptr);
    }

    EXPECT_EQ(RegistryRegistrator::GetComponentsManager("test_null"), nullptr);
}

