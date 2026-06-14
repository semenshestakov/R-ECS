#pragma once
#include "ecs/ISystem.hpp"
#include "ecs/systems/EventSystem.hpp"


template<typename SystemCls>
ecs::IBaseSystem* ecs::ISystem<SystemCls>::New() const
{
    return new SystemCls();
}

template<typename SystemCls>
std::string_view ecs::ISystem<SystemCls>::name() const
{
    return typeid(SystemCls).name();
}

template<typename SystemCls>
void ecs::ISystem<SystemCls>::Init(const InitState& state)
{
    IBaseSystem::Init(state);
    (void) (SystemCls::RegisterInfo);
}

template<typename SystemCls>
void ecs::ISystem<SystemCls>::Subscribe(const SubscribeState& state)
{
    for(const auto& registerFunction: m_registerEventFunctions)
    {
        registerFunction(state.eventSystem, state.priority);
    }
    m_registerEventFunctions.clear();
}

template<typename SystemCls>
template<class Event>
auto ecs::ISystem<SystemCls>::RegisterEvent(
    void (SystemCls::*method)(Registry&, const Event&)
    )-> std::unique_ptr<EventListener<Event>>
{
    std::unique_ptr<EventListener<Event>> listenerPtr = std::make_unique<EventListener<Event>>();
    std::function<void(EventSystem&, event::priority_t)> registerFunction = [this, listener = listenerPtr.get(), method](
        EventSystem& eventSystem, event::priority_t priority
        )
    {
        eventSystem.Create<Registry&, const Event&>(GetEventKey<Event>());
        listener->subscribe(
            eventSystem.TryGet<Registry&, const Event&>(GetEventKey<Event>()),
            [this, method](Registry& registry, const Event& event)
            {
                (static_cast<SystemCls*>(this)->*method)(registry, event);
            },
            priority);
    };
    m_registerEventFunctions.emplace_back(registerFunction);
    return listenerPtr;
}

template<typename SystemCls>
template<typename... SystemsArgs>
constexpr std::array<ecs::systemHash_t, sizeof...(SystemsArgs)> ecs::ISystem<SystemCls>::GetSystemsHashArray()
{
    return {ecs::getSystemHash<SystemsArgs>()...};
}

template<typename SystemCls>
template<typename... Accessors>
std::array<ecs::ComponentAccessEntry, sizeof...(Accessors)> ecs::ISystem<SystemCls>::GetComponentAccessArray()
{
    return {ecs::ComponentAccessEntry{ecs::getComponentHash<typename Accessors::component>(), Accessors::kind}...};
}
