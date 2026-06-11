/**
 * @file Entity.hpp
 * @brief Core entity handle and entity concept definition
 */

#pragma once
#include "ecs/utils/EntitiesUtils.hpp"


namespace ecs
{

    /**
     * @brief Concept for types that behave as entity handles.
     *
     * A type satisfies EntityConcept if it provides:
     * - getId() returning entityId_t
     * - getVersion() returning entityVersion_t
     *
     * @note Entity, EntityWrapper, and any wrapper-derived types satisfy this concept.
     */
    template<typename T> concept EntityConcept = requires(T entity)
    {
        { entity.getId()     } -> std::same_as<entityId_t>;
        { entity.getVersion()} -> std::same_as<entityVersion_t>;
    };


    struct Entity
    {
        entityId_t id = INVALID_ENTITY_ID;                  ///< Unique identifier for the entity across the entire system
        entityVersion_t version = INVALID_ENTITY_VERSION;   ///< Unique identifier for this->id

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

        [[nodiscard]] entityId_t getId() const { return id; }
        [[nodiscard]] entityVersion_t getVersion() const { return version; }
    };

}
