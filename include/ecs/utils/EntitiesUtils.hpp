#pragma once
#include <cstdint>


namespace ecs
{

    using entityId_t = std::uint32_t;
    constexpr entityId_t INVALID_ENTITY_ID = 0;
    constexpr entityId_t MAX_ENTITY_ID = ~0u;

    using entityVersion_t = std::uint32_t;
    constexpr entityVersion_t INVALID_ENTITY_VERSION = 0;
    constexpr entityVersion_t MAX_ENTITY_VERSION = ~0u;

}