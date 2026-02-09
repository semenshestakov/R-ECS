#include <gtest/gtest.h>
#include "ecs/IComponent.hpp"
#include "components_class.hpp"


using namespace ecs;

TEST(Components, RegisterComponentId)
{
    EXPECT_EQ(Position2d::componentId, 1);
    EXPECT_EQ(Position3d::componentId, 2);
}


TEST(Components, SimpleRegistration)
{
    ComponentsManager factory;
    factory.Register<Position2d>();
    factory.Register<Position3d>();

    {
        ComponentsPtr components = factory.CreateComponents();
        components->initialize();
        EXPECT_EQ(components->mustGet<Position3d>().x, Position3d().x);

        components->mustGet<Position3d>().x += 5.0;
        EXPECT_EQ(components->mustGet<Position3d>().x, Position3d().x + 5.0);
    }

    {
        ComponentsPtr components = factory.CreateComponents();
        components->init<Position2d>(Position2d(10.0, 20.0));
        components->initialize();

        EXPECT_EQ(components->get<Position2d>()->x, 10.0);

        components->get<Position2d>()->y = 30.0;
        EXPECT_EQ(components->get<Position2d>()->y, 30.0);
    }
}


TEST(Components, Destructor)
{
    ComponentsManager factory;
    factory.Register<DestructorTest>();

    {
        ComponentsPtr components = factory.CreateComponents();
        components->initialize();

        EXPECT_EQ(DestructorTest::testValue, false);
    }

    {
        ComponentsPtr components = factory.CreateComponents();
        components->initialize();

        EXPECT_EQ(DestructorTest::testValue, true);
    }
}

TEST(Components, Copy)
{
#ifdef DEEP_TEST_ENABLE
    ComponentsManager factory;
    {
        ComponentsManager newFactory;
        newFactory.Register<Position2d>();
        newFactory.Register<Position3d>();
        factory.copy(newFactory);
    }

    {
        ComponentsPtr components = factory.CreateComponents();
        components->initialize();
        EXPECT_EQ(components->mustGet<Position3d>().x, Position3d().x);
    }
#endif
}
