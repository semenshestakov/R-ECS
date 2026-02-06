#include <gtest/gtest.h>
#include "ComponentsClass.hpp"
#include "ecs/IComponent.hpp"


using namespace ecs;


TEST(ComponentsTest, SimpleRegistration)
{
    ComponentsManager componentsManager;
    componentsManager.Register<Position2d>();
    componentsManager.Register<Position3d>();

    {
        ComponentsPtr components = componentsManager.CreateComponents();
        components->initialize();
        EXPECT_EQ(components->mustGet<Position3d>().x, Position3d().x);

        components->mustGet<Position3d>().x += 5.0;
        EXPECT_EQ(components->mustGet<Position3d>().x, Position3d().x + 5.0);
    }

    {
        ComponentsPtr components = componentsManager.CreateComponents();
        components->init<Position2d>(Position2d(10.0, 20.0));
        components->initialize();

        EXPECT_EQ(components->get<Position2d>()->x, 10.0);

        components->get<Position2d>()->y = 30.0;
        EXPECT_EQ(components->get<Position2d>()->y, 30.0);
    }
}


TEST(ComponentsTest, Destructor)
{
    ComponentsManager componentsManager;
    componentsManager.Register<DestructorTest>();

    {
        ComponentsPtr components = componentsManager.CreateComponents();
        components->initialize();

        EXPECT_EQ(DestructorTest::testValue, false);
    }

    {
        ComponentsPtr components = componentsManager.CreateComponents();
        components->initialize();

        EXPECT_EQ(DestructorTest::testValue, true);
    }
}


TEST(ComponentsTest, Copy)
{
    ComponentsManager componentsManager;
    {
        ComponentsManager newFactory;
        newFactory.Register<Position2d>();
        newFactory.Register<Position3d>();
        componentsManager.copy(newFactory);
    }

    ComponentsPtr components = componentsManager.CreateComponents();
    components->initialize();
    EXPECT_EQ(components->mustGet<Position3d>().x, Position3d().x);

}
