#pragma once
#include "entities/EntityWrapper.hpp"


namespace ecs
{

    struct GameEntity : EntityWrapper
    {
        using EntityWrapper::EntityWrapper;
    };

}