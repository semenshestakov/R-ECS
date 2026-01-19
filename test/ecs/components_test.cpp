#include <gtest/gtest.h>
#include "components_class.hpp"
#include "ecs/RegistryFactory.hpp"


using namespace ecs;

TEST(Components, RegisterComponentId)
{
    EXPECT_EQ(Position2d::componentId, 1);
    EXPECT_EQ(Position3d::componentId, 2);
}


TEST(FactoryComponents, SimpleRegistration)
{
    RegistryFactory factory;
    factory.Register<Position2d>();
    factory.Register<Position3d>();

    {
        ComponentsPtr components = factory.createComponents();
        components->initialize();
        EXPECT_EQ(components->mustGet<Position3d>().x, Position3d().x);

        components->mustGet<Position3d>().x += 5.0;
        EXPECT_EQ(components->mustGet<Position3d>().x, Position3d().x + 5.0);
    }

    {
        ComponentsPtr components = factory.createComponents();
        components->init<Position2d>(Position2d(10.0, 20.0));
        components->initialize();

        EXPECT_EQ(components->get<Position2d>()->x, 10.0);

        components->get<Position2d>()->y = 30.0;
        EXPECT_EQ(components->get<Position2d>()->y, 30.0);
    }
}


TEST(ComponentsDestructor, SimpleRegistration)
{
    RegistryFactory factory;
    factory.Register<DestructorTest>();

    {
        ComponentsPtr components = factory.createComponents();
        components->initialize();

        EXPECT_EQ(DestructorTest::testValue, false);
    }

    {
        ComponentsPtr components = factory.createComponents();
        components->initialize();

        EXPECT_EQ(DestructorTest::testValue, true);
    }
}
