#pragma once
#include "../Commands.hpp"
#include "../Events.hpp"
#include "ecs/Registry.hpp"

template <typename E, ecs::Recipe R>
void ecs::CookCmd<E, R>::operator()(Registry& registry) const
{
    PrefabEntity prefab;

    recipe.apply(prefab);
    if ((feedback & CookFeedback::PRE_EVT_CALL) != CookFeedback::NONE)
    {
        registry.Events().OnEvent<PrefabEvt<E>>({prefab});
    }

    auto entity = registry.Entities().Create<E>(std::move(prefab));

    if ((feedback & CookFeedback::POST_EVT_CALL) != CookFeedback::NONE)
    {
        registry.Events().OnEvent<CreatedEntityEvt<E>>({entity});
    }
}

template<typename... Args>
void ecs::AddComponentsCmd<Args...>::operator()(Registry& registry) const
{
    std::apply(
        [&](Args&... comps)
        {
            registry.Entities().AddComponents(entity, std::move(comps)...);
        },
        components);
}

template<ecs::IsComponent... Args>
void ecs::RemoveComponentsCmd<Args...>::operator()(Registry& registry) const
{
    registry.Entities().RemoveComponents<Args...>(entity);
}

inline ecs::CookFeedback operator|(ecs::CookFeedback v1, ecs::CookFeedback v2)
{
    return static_cast<ecs::CookFeedback>(static_cast<std::uint8_t>(v1) | static_cast<std::uint8_t>(v2));
}

inline ecs::CookFeedback operator&(ecs::CookFeedback v1, ecs::CookFeedback v2)
{
    return static_cast<ecs::CookFeedback>(static_cast<std::uint8_t>(v1) & static_cast<std::uint8_t>(v2));
}
