#include "components_class.hpp"
#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/entities/UMapEntitiesManager.hpp"
#include "ecs/entities/ranges/EntitiesViews.hpp"
#include "ecs/systems/SystemManager.hpp"
#include "gtest/gtest.h"


using namespace ecs;


struct SystemTestUpdate final : ISystem<SystemTestUpdate>
{
    inline static unsigned int Counter = {0};

    void Update(Registry& registry, const UpdateState& state) override
    {
        Counter++;
    }
};

struct SystemTestId final : ISystem<SystemTestId>
{
    void Update(Registry& registry, const UpdateState& state) override
    {
        for (auto& components : registry.view())
        {
            components.mustGet<TestId>().id++;
        }
    }
};


TEST(Systems, CreateSystems)
{
    Registry registry;
    {
        ComponentsManager* factory = RegistryRegistrator::GetComponentsManager("test1");  // PositionX, PositionY, TestId
        SystemManager systemManger;
        systemManger.Register<SystemTestUpdate>();
        systemManger.Register<SystemTestId>();

        registry =  Registry(EntitiesManager::Create<UMapEntitiesManager>(), *factory, systemManger);
    }

    {
        for (std::size_t i = 1; i < 100; i++)
        {
            registry.Update();
            EXPECT_EQ(SystemTestUpdate::Counter, i);
        }
    }


    {
        constexpr std::size_t MAX_ENTITIES = 20;
        for (std::size_t i = 0; i < MAX_ENTITIES; i++)
        {
            Components& components = registry.Create(i);
            EXPECT_EQ(components.mustGet<TestId>().id, TestId().id);
            registry.Update();
            EXPECT_EQ(components.mustGet<TestId>().id, TestId().id + 1);
        }

        for (std::size_t i = 0; i < MAX_ENTITIES; i++)
        {
            Components& components = registry.mustGet(i);
            EXPECT_EQ(components.mustGet<TestId>().id, TestId().id + MAX_ENTITIES - i);
        }
    }

}