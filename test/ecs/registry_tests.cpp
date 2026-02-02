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
