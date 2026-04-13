#pragma once
#include "ecs/ISystem.hpp"


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
    (void) (SystemCls::IsRegistered);
    for(const auto& registerFunction: m_registerEventFunctions)
    {
        registerFunction(state.eventSystem);
    }
    m_registerEventFunctions.clear();
}

template<typename SystemCls>
template<class Event>
auto ecs::ISystem<SystemCls>::RegisterEvent(void (SystemCls::*method)(Registry&, const Event&))
        -> std::unique_ptr<EventListener<Event>>
{
    std::unique_ptr<EventListener<Event>> listenerPtr = std::make_unique<EventListener<Event>>();
    std::function<void(EventSystem&)> registerFunction = [this, listener = listenerPtr.get(),
                                                          method](EventSystem& eventSystem) {
        eventSystem.Create<Registry&, const Event&>(TryGetKey<Event>());
        listener->subscribe(
            eventSystem.TryGet<Registry&, const Event&>(TryGetKey<Event>()),
            [this, method](Registry& registry, const Event& event)
            {
                (static_cast<SystemCls*>(this)->*method)(registry, event);
            });
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
