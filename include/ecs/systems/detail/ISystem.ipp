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
    if(m_subscribed)
        return;

    for(const auto& registerFunction: m_registerEventFunctions)
    {
        registerFunction(state.eventSystem, state.priority);
    }
    m_subscribed = true;
}

template<typename SystemCls>
void ecs::ISystem<SystemCls>::Unsubscribe()
{
    if(!m_subscribed)
        return;

    for(event::AbstractListener<event::callbackId_t>* listener: m_eventListeners)
    {
        if(listener != nullptr)
            listener->clear();   // also resets the listener's event ptr so Subscribe() can re-attach
    }
    m_subscribed = false;
}

template<typename SystemCls>
void ecs::ISystem<SystemCls>::SetEventsPriority(const event::priority_t priority)
{
    for(event::AbstractListener<event::callbackId_t>* listener: m_eventListeners)
    {
        if(listener != nullptr)
            listener->setPriority(priority);
    }
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
    m_eventListeners.emplace_back(listenerPtr.get());
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
