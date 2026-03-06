#pragma once
#include "ecs/ISystem.hpp"


namespace ecs
{

    template<typename SystemCls>
    void ISystem<SystemCls>::Init(const InitState& state)
    {
        IBaseSystem::Init(state);
        (void)(SystemCls::IsRegistered);
        for (const auto& registerFunction: m_registerEventFunctions)
        {
            registerFunction(state.eventSystem);
        }
        m_registerEventFunctions.clear();
    }

    template<typename SystemCls>
    template<class Event>
    auto ISystem<SystemCls>::RegisterEvent(void (SystemCls::*method)(Registry&, const Event&)) -> std::unique_ptr<EventListener<Event>>
    {
        std::unique_ptr<EventListener<Event>> listenerPtr = std::make_unique<EventListener<Event>>();
        std::function<void(EventSystem&)> registerFunction = [this, listener=listenerPtr.get(), method](EventSystem& eventSystem)
        {
            eventSystem.Create<Registry&, const Event&>(getEventKey<Event>());
            listener->subscribe(
                eventSystem.get<Registry&, const Event&>(getEventKey<Event>()),
                [this, method](Registry& registry, const Event& event){ (static_cast<SystemCls*>(this)->*method)(registry, event); }
                );
        };
        m_registerEventFunctions.emplace_back(registerFunction);
        return listenerPtr;
    }

}