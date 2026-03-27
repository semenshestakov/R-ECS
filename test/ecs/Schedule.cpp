#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/systems/SystemsManager.hpp"
#include "gtest/gtest.h"
#include "SystemsClass.hpp"


using namespace ecs;


class ScheduleSystemTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_systemCallOrder.clear();

        registry = Registry(*RegistryRegistrator::GetSystemsManager("test_schedule"));
        registry.Init();
    }

    Registry registry = Registry();
};


TEST_F(ScheduleSystemTest, DependentSystemOrder)
{
    registry.Update();

    const auto itRender = std::find(g_systemCallOrder.begin(), g_systemCallOrder.end(), "RenderSystem");
    const auto itResource = std::find(g_systemCallOrder.begin(), g_systemCallOrder.end(), "ResourceSystem");
    const auto itInput = std::find(g_systemCallOrder.begin(), g_systemCallOrder.end(), "InputSystem");
    const auto itPhysics = std::find(g_systemCallOrder.begin(), g_systemCallOrder.end(), "PhysicsSystem");

    ASSERT_TRUE(itRender != g_systemCallOrder.end());
    ASSERT_TRUE(itResource != g_systemCallOrder.end());
    ASSERT_TRUE(itInput != g_systemCallOrder.end());
    ASSERT_TRUE(itPhysics != g_systemCallOrder.end());

    EXPECT_LT(std::distance(g_systemCallOrder.begin(), itResource), std::distance(g_systemCallOrder.begin(), itRender));
    EXPECT_LT(std::distance(g_systemCallOrder.begin(), itInput), std::distance(g_systemCallOrder.begin(), itRender));
    EXPECT_LT(std::distance(g_systemCallOrder.begin(), itPhysics), std::distance(g_systemCallOrder.begin(), itRender));
}


TEST_F(ScheduleSystemTest, IndependentSystemOrder)
{
    registry.Update();

    const auto itAI = std::ranges::find(g_systemCallOrder, "AISystem");
    const auto itRender = std::ranges::find(g_systemCallOrder, "RenderSystem");

    ASSERT_TRUE(itAI != g_systemCallOrder.end());
    ASSERT_TRUE(itRender != g_systemCallOrder.end());
}


TEST_F(ScheduleSystemTest, FullCallSequence)
{
    registry.Update();

    const std::vector<std::string> expectedSystems = {
        "ResourceSystem", "InputSystem", "PhysicsSystem", "RenderSystem", "AISystem"
    };

    for (auto& sys : expectedSystems)
    {
        EXPECT_NE(std::ranges::find(g_systemCallOrder.begin(), g_systemCallOrder.end(), sys), g_systemCallOrder.end())
            << sys << " is not Update!";
    }
}