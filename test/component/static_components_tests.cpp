#include <gtest/gtest.h>
#include "components_class.hpp"
#include "ecs/component/StaticComponentsFactory.hpp"

using namespace ecs;
using namespace ecs::component::error;


TEST(StaticComponensTest, AddGet_no_inheritance)
{
    StaticComponentsFactory factory;
    EXPECT_NO_THROW({
        factory.Register<Position3D_no_inheritance>();
        factory.Register<Position2D_no_inheritance>();
        });

    {
        StaticComponentsPtr components = factory.createComponents();
        EXPECT_EQ(components->get<Position3D_no_inheritance>(), nullptr);

        EXPECT_EQ(components->init<Position3D_no_inheritance>(-1, 2 ,3), true);
        EXPECT_NE(components->get<Position3D_no_inheritance>(), nullptr);

        EXPECT_EQ(components->get<Position3D_no_inheritance>()->x, -1);
        EXPECT_EQ(components->get<Position3D_no_inheritance>()->y, 2);
        EXPECT_EQ(components->get<Position3D_no_inheritance>()->z, 3);

        components->get<Position3D_no_inheritance>()->x = 4;
        components->get<Position3D_no_inheritance>()->z = 5;
        EXPECT_EQ(components->get<Position3D_no_inheritance>()->x, 4);
        EXPECT_EQ(components->get<Position3D_no_inheritance>()->y, 2);
        EXPECT_EQ(components->get<Position3D_no_inheritance>()->z, 5);

        EXPECT_EQ(components->get<Position2D_no_inheritance>(), nullptr);
        components->initialize();

        EXPECT_NE(components->get<Position2D_no_inheritance>(), nullptr);

        EXPECT_EQ(components->get<Position2D_no_inheritance>()->x, 10);
        EXPECT_EQ(components->get<Position2D_no_inheritance>()->y, 12);

        components->get<Position2D_no_inheritance>()->x = -10;
        EXPECT_EQ(components->get<Position2D_no_inheritance>()->x, -10);
        EXPECT_EQ(components->get<Position2D_no_inheritance>()->y, 12);

        EXPECT_EQ(components->get<Position3D_no_inheritance>()->x, 4);
        EXPECT_EQ(components->get<Position3D_no_inheritance>()->y, 2);
        EXPECT_EQ(components->get<Position3D_no_inheritance>()->z, 5);

    }
}

