#pragma once
#include "ecs/ISystem.hpp"
#include "ecs/registry/AutoRegistry.hpp"
#include "event/Listener.hpp"
#include "event/SmartListener.hpp"


struct Event1
{
    int value = 0;
};

struct Event2
{
    int value = 1;
};



struct SystemEventHandler1 final : ecs::ISystem<SystemEventHandler1>
{
    ECS_REGISTRY("test_event")

    static inline bool IsUpdated = false;
    void Update(ecs::Registry& registry, const ecs::UpdateState & state) override { IsUpdated = true;}

};


struct SystemEventHandler2 final : ecs::ISystem<SystemEventHandler2>
{
    ECS_REGISTRY("test_event")

    static inline bool IsUpdated = false;
    void Update(ecs::Registry& registry, const ecs::UpdateState & state) override { IsUpdated = true;}

    void OnEvent(ecs::Registry& registry, const Event1& event);

    void OnEvent(ecs::Registry& registry, const Event2& event);

private:
    // ECS_LISTENER(Event2); //
    event::SingleListener<ecs::Registry&, const Event2&>* __m_listener_Event2 = {};
public:
};


