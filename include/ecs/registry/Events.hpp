#pragma once
#include <functional>
#include "ecs/entities/EntityWrapper.hpp"
#include "ecs/entities/PrefabEntity.hpp"


namespace ecs
{

    /**
     * @brief Event payload for deferred entity creation from a recipe.
     *
     * @tparam E The EntityWrapper-derived type to create
     *
     * Fired by CookCommand via EventSystem::OnEvent().
     * A subscribed handler typically reads the prefab and creates the entity
     * (directly or via another deferred command).
     */
    template<EntityWrapperLike E>
    struct PrefabEvt
    {
        PrefabEntity& prefab;
    };

    template<EntityWrapperLike E>
    struct CreatedEntityEvt
    {
        E& entity;
    };

} // namespace ecs
