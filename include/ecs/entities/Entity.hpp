#pragma once
#include "ecs/utils/EntitiesUtils.hpp"


namespace ecs
{

    struct Entity
    {
        entityId_t id = INVALID_ENTITY_ID;                  ///< Unique identifier for the entity across the entire system
        entityVersion_t version = INVALID_ENTITY_VERSION;

        /**
         * @brief Three-way comparison operator for ordering entities by their IDs.
         *
         * Enables all six relational operators (==, !=, <, <=, >, >=) for entities
         * through a single implementation. Entities are ordered based on their unique
         * identifiers, which is useful for sorting containers, using entities as keys
         * in ordered maps, or performing set operations on entity collections.
         *
         * @param other The other entity to compare against
         * @return auto The result of comparing the entity IDs (strong ordering)
         */
        auto operator<=>(const Entity& other) const  { return std::tie(id, version) <=> std::tie(other.id, other.version); }
    };
}
