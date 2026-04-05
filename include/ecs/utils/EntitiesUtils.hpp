#pragma once
#include <cstdint>


namespace ecs
{

    /**
     * @brief Unique identifier for an entity.
     * Entity ID is an index into the manager's internal arrays.
     * IDs are recycled after entity destruction with version incrementation
     * to detect stale handles.
     *
     * @note ID 0 (INVALID_ENTITY_ID) is reserved for invalid/null entities.
     * @note Maximum value is ~0u (all bits set), providing up to ~4.2 billion entities.
     */
    using entityId_t = std::uint32_t;

    /**
     * @brief Sentinel value indicating an invalid/null entity ID.
     * Entity handles with this ID should be considered invalid.
     */
    constexpr entityId_t INVALID_ENTITY_ID = 0;

    /**
     * @brief Maximum possible entity ID value.
     * Represents the theoretical maximum number of entities that can
     * exist simultaneously (though practical limits depend on memory).
     */
    constexpr entityId_t MAX_ENTITY_ID = ~0u;

    /**
     * @brief Version counter for entity handle validation.
     * Each time an entity ID is reused, its version increments.
     * Combined with entity ID, this forms a handle that can detect
     * use-after-free bugs when an old handle references a recycled slot.
     *
     * @note Version 0 (INVALID_ENTITY_VERSION) is reserved for invalid entities.
     * @note Versions wrap around after reaching MAX_ENTITY_VERSION.
     */
    using entityVersion_t = std::uint32_t;

    /**
     * @brief Sentinel value indicating an invalid entity version.
     * Entity handles with this version should be considered invalid.
     */
    constexpr entityVersion_t INVALID_ENTITY_VERSION = 0;

    /**
     * @brief Maximum possible entity version value.
     * After reaching this value, version wraps back to 1 (skipping 0).
     * This provides up to ~4.2 billion reuse cycles per entity slot.
     */
    constexpr entityVersion_t MAX_ENTITY_VERSION = ~0u;

}