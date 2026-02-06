#include "components_class.hpp"
#include "ecs/Registry.hpp"
#include "ecs/RegistryUMap.hpp"
#include "gtest/gtest.h"

using namespace ecs;


TEST(RegistryUMapTest, CreateWithGeneratedId)
{
    RegistryFactory factory;
    factory.Register<Position2d>();
    factory.Register<Position3d>();

    RegistryUMap registry(factory);

    {
        auto& components = registry.create();
        EXPECT_NE(registry.get(1), nullptr);
    }

    {
        auto& components = registry.create();
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
    RegistryFactory factory;
    factory.Register<Position2d>();
    factory.Register<Position3d>();

    RegistryUMap registry(factory);

    {
        auto& components = registry.create(100);
        EXPECT_NE(registry.get(100), nullptr);
    }

    {
        auto& components = registry.create(200);
        EXPECT_NE(registry.get(200), nullptr);
    }

    EXPECT_THROW(
        {
            auto& components = registry.create(100);
        },
        error::InvalidEntityId);
}



struct PositionX : IComponent<PositionX>
{
    ECS_FACTORY("test1", "test2")
    float x {1};
};

struct PositionY : IComponent<PositionY>
{
    ECS_FACTORY("test1")
    float y {2};
};

struct PositionZ : IComponent<PositionZ>
{
    float z {3};
};


TEST(Components, AutoRegistration)
{
#ifdef DEEP_TESTS_TEST_ENABLE
    EXPECT_EQ(RegistryFactoryRegistrator::Registrator::size(), 2);
    EXPECT_EQ(PositionX::IsRegistered, true);
    EXPECT_EQ(PositionY::IsRegistered, true);
    EXPECT_EQ(PositionZ::IsRegistered, false);
#endif


    RegistryFactory* factory = RegistryFactoryRegistrator::GetFactory("test1");
    EXPECT_NE(factory, nullptr);
    {
        ComponentsPtr components = factory->CreateComponents();
        components->initialize();

        EXPECT_EQ(components->get<PositionX>()->x, PositionX().x);
        EXPECT_EQ(components->get<PositionY>()->y, PositionY().y);

        EXPECT_EQ(components->get<PositionZ>(), nullptr);
    }

    EXPECT_EQ(RegistryFactoryRegistrator::GetFactory("test_null"), nullptr);
}

