#include <gtest/gtest.h>
#include "components_class.hpp"
#include "ecs/component/DynamicComponentsFactory.hpp"

using namespace ecs;
using namespace ecs::component::error;


TEST(DynamicComponentsTest, AddGet)
{
    DynamicComponentsFactory factory;
    EXPECT_NO_THROW(
        {
            factory.add<Position2D_no_inheritance>(1, 2);
            factory.add<Position3D_inheritance>(3, 4, 5);
        });

    // no_inheritance
    EXPECT_EQ(factory.get<Position2D_no_inheritance>()->x, 1);
    EXPECT_EQ(factory.get<Position2D_no_inheritance>()->y, 2);

    // inheritance N1
    EXPECT_EQ(factory.get<Position3D_inheritance>()->x, 3);
    EXPECT_EQ(factory.get<Position3D_inheritance>()->y, 4);
    EXPECT_EQ(factory.get<Position3D_inheritance>()->z, 5);

    // inheritance N2
    EXPECT_EQ(factory.get<Position2D_inheritance>()->x, 3);
    EXPECT_EQ(factory.get<Position2D_inheritance>()->y, 4);

    // add THROW eq componentsId
    EXPECT_THROW(
        {
        factory.add<Position2D_inheritance>(1, 2);

        }, BaseComponentError);
}

// TestDestructor
class _TestDestructor1 : public BaseComponent
{
public:
    static constexpr componentId_t componentId = ComponentsType::DESTRUCTOR;
    bool& testBool1;
    _TestDestructor1(bool& a_testBool) : testBool1(a_testBool) {}
    ~_TestDestructor1() override { testBool1 = true; }
};

class _TestDestructor2 : public _TestDestructor1
{
    public:
        bool& testBool2;
        _TestDestructor2(bool& a_testBool1, bool& a_testBool2) : _TestDestructor1(a_testBool1), testBool2(a_testBool2) {}
        ~_TestDestructor2() override { testBool2 = true; }
};

TEST(DynamicComponentsTest, Destructor)
{
    // delete EXPECT_NO_THROW
    EXPECT_NO_THROW({
        DynamicComponentsFactory* factory = new DynamicComponentsFactory(100);

        EXPECT_NO_THROW(
            delete factory
            );
    });

    // delete EXPECT_THROW ~_TestDestructor1()
    EXPECT_NO_THROW({
        bool testBool = false;

        DynamicComponentsFactory* factory = new DynamicComponentsFactory(100);
        factory->add<_TestDestructor1>(testBool);
        delete factory;

        EXPECT_EQ(testBool, true);
    });

    // delete EXPECT_THROW ~_TestDestructor1() add ~_TestDestructor2()
    EXPECT_NO_THROW({
        bool testBool1 = false;
        bool testBool2 = false;

        DynamicComponentsFactory* factory = new DynamicComponentsFactory(100);
        factory->add<_TestDestructor2>(testBool1, testBool2);
        delete factory;

        EXPECT_EQ(testBool1, true);
        EXPECT_EQ(testBool2, true);
    });
}
