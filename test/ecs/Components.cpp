#include <vector>
#include <new>
#include <cstddef>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
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


struct InitCounter : IComponent<InitCounter>
{
    static int constructorCalls;
    static int destructorCalls;
    static int copyCalls;
    static int moveCalls;

    int id;
    float value;

    InitCounter() : id(++constructorCalls), value(42.0f) {}

    InitCounter(float v) : id(++constructorCalls), value(v) {}

    InitCounter(const InitCounter& other) : id(other.id), value(other.value) {
        copyCalls++;
    }

    InitCounter(InitCounter&& other) : id(other.id), value(other.value) {
        moveCalls++;
    }

    ~InitCounter() {
        destructorCalls++;
    }

    static void reset() {
        constructorCalls = 0;
        destructorCalls = 0;
        copyCalls = 0;
        moveCalls = 0;
    }
};

int InitCounter::constructorCalls = 0;
int InitCounter::destructorCalls = 0;
int InitCounter::copyCalls = 0;
int InitCounter::moveCalls = 0;

TEST(ComponentsReinitTest, SingleComponentReinit)
{
    ComponentsManager manager;
    manager.Register<InitCounter>();

    InitCounter::reset();

    auto components = manager.CreateComponents();

    bool result1 = components->init<InitCounter>(10.0f);
    EXPECT_TRUE(result1);

    auto* comp1 = components->get<InitCounter>();
    ASSERT_NE(comp1, nullptr);
    EXPECT_FLOAT_EQ(comp1->value, 10.0f);
    EXPECT_EQ(InitCounter::constructorCalls, 1);
    EXPECT_EQ(InitCounter::destructorCalls, 0);

    bool result2 = components->init<InitCounter>(20.0f);
    EXPECT_TRUE(result2);

    auto* comp2 = components->get<InitCounter>();
    ASSERT_NE(comp2, nullptr);
    EXPECT_FLOAT_EQ(comp2->value, 20.0f);

    EXPECT_EQ(InitCounter::constructorCalls, 2);
    EXPECT_EQ(InitCounter::destructorCalls, 1);

    EXPECT_EQ(comp1, comp2);
}


TEST(ComponentsReinitTest, MultipleReinitCalls)
{
    ComponentsManager manager;
    manager.Register<InitCounter>();

    InitCounter::reset();

    auto components = manager.CreateComponents();

    for (int i = 0; i < 5; ++i)
    {
        components->init<InitCounter>(float(i * 10));

        auto* comp = components->get<InitCounter>();
        EXPECT_FLOAT_EQ(comp->value, float(i * 10));
    }

    EXPECT_EQ(InitCounter::constructorCalls, 5);
    EXPECT_EQ(InitCounter::destructorCalls, 4);
}


TEST(ComponentsReinitTest, ReinitAfterInitialize)
{
    ComponentsManager manager;
    manager.Register<InitCounter>();
    manager.Register<Position2d>();

    InitCounter::reset();

    auto components = manager.CreateComponents();
    components->initialize();

    auto* counter = components->get<InitCounter>();
    auto* pos = components->get<Position2d>();

    if (counter)
    {
        EXPECT_FLOAT_EQ(counter->value, 42.0f);
    }

    components->init<InitCounter>(99.9f);

    auto* newCounter = components->get<InitCounter>();
    EXPECT_FLOAT_EQ(newCounter->value, 99.9f);

    if (pos) {
    }
}

TEST(ComponentsReinitTest, ReinitWithDifferentTypes)
{
    ComponentsManager manager;
    manager.Register<InitCounter>();
    manager.Register<Position2d>();
    manager.Register<Position3d>();

    InitCounter::reset();

    auto components = manager.CreateComponents();

    components->init<InitCounter>(1.0f);
    components->init<Position2d>(10.0f, 20.0f);
    components->init<Position3d>();

    auto* counter1 = components->get<InitCounter>();
    auto* pos1 = components->get<Position2d>();

    EXPECT_FLOAT_EQ(counter1->value, 1.0f);
    EXPECT_FLOAT_EQ(pos1->x, 10.0f);

    components->init<Position2d>(30.0f, 40.0f);

    auto* pos2 = components->get<Position2d>();
    EXPECT_FLOAT_EQ(pos2->x, 30.0f);
    EXPECT_FLOAT_EQ(pos2->y, 40.0f);

    auto* counter2 = components->get<InitCounter>();
    EXPECT_EQ(counter1, counter2);
    EXPECT_FLOAT_EQ(counter2->value, 1.0f);
}


TEST(ComponentsReinitTest, DestructorCalledOnReinit)
{
    ComponentsManager manager;
    manager.Register<InitCounter>();

    InitCounter::reset();

    {
        auto components = manager.CreateComponents();

        components->init<InitCounter>();
        EXPECT_EQ(InitCounter::constructorCalls, 1);
        EXPECT_EQ(InitCounter::destructorCalls, 0);

        components->init<InitCounter>();
        EXPECT_EQ(InitCounter::constructorCalls, 2);
        EXPECT_EQ(InitCounter::destructorCalls, 1);
    }

    EXPECT_EQ(InitCounter::destructorCalls, 2);
}

TEST(ComponentsReinitTest, InitializeAfterReinit)
{
    ComponentsManager manager;
    manager.Register<InitCounter>();

    InitCounter::reset();

    auto components = manager.CreateComponents();

    components->init<InitCounter>(5.0f);

    components->initialize();

    auto* counter = components->get<InitCounter>();
    EXPECT_FLOAT_EQ(counter->value, 5.0f);

    EXPECT_EQ(InitCounter::constructorCalls, 1);
    EXPECT_EQ(InitCounter::destructorCalls, 0);
}
