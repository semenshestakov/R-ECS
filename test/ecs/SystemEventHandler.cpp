#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "SystemsClass.hpp"
#include "ecs/entities/UMapEntitiesManager.hpp"


using namespace ecs;
using ::testing::_;


class SystemEventHandlerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        registry = Registry(
            EntitiesManager::Create<UMapEntitiesManager>(),
            *RegistryRegistrator::GetComponentsManager("test_event"),
            *RegistryRegistrator::GetSystemsManager("test_event")
        );
        registry.Init();
    }

    Registry registry = Registry();
};


TEST_F(SystemEventHandlerTest, TestCountEvents)
{
    EXPECT_EQ(registry.m_eventSystem.size(), 2);
}


TEST_F(SystemEventHandlerTest, CallEvent1)
{
    EXPECT_CALL(*registry.getSystem<SystemEventHandler2>(), callInt(100)).Times(1);
    EXPECT_CALL(*registry.getSystem<SystemEventHandler1>(), callInt(100)).Times(1);
    registry.onEvent<Event1>(Event1(100));
}


TEST_F(SystemEventHandlerTest, CallEvent2)
{
    EXPECT_CALL(*registry.getSystem<SystemEventHandler2>(), callInt(100)).Times(1);
    EXPECT_CALL(*registry.getSystem<SystemEventHandler1>(), callInt(100)).Times(0);
    registry.onEvent<Event2>(Event2(100));
}
