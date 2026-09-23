#pragma once
#include <concepts>
#include <cstdint>
#include "common_recs/utils/BaseError.hpp"


namespace ecs
{
    struct EntityWrapper;

    // Basic type aliases for memory management and component identification
    using byte = std::byte;               ///< Fundamental byte type for raw memory operations
    using bufferSize_t = unsigned int;    ///< Type for representing buffer sizes and memory capacities
    using componentId_t = std::uint32_t;  ///< Type for unique component type identifiers

    // Special value indicating an invalid or uninitialized component ID
    constexpr componentId_t INVALID_COMPONENT_ID = 0;
    constexpr componentId_t MAX_COMPONENT_ID = ~0;
    constexpr unsigned int OVERFLOW_MAX_COMPONENT_ID = static_cast<unsigned int>(MAX_COMPONENT_ID) + 1;

    /**
     * @brief Empty marker base for zero-sized archetype tags.
     *
     * A tag participates in an entity's archetype (occupies a bit) but stores no
     * per-entity data. It is stored as a component whose componentSize is 0, so no
     * chunk column is allocated for it. Being empty, it is folded away by EBO when
     * combined with EntityWrapper, so wrapper-tags keep sizeof(EntityWrapper).
     */
    struct Tag {};

    /**
     * @brief Concept satisfied by any type deriving from ecs::Tag, except EntityWrapper.
     *
     * Drives compile-time branching in the view machinery and in the registrator:
     * a tag is filter-only (contributes an archetype bit, never a data column and
     * never an element in the yielded tuple).
     *
     * EntityWrapper derives from Tag so that every *named* wrapper subclass is a tag,
     * but the base wrapper itself is carved out here — it is the one wrapper head that
     * adds no filter.
     */
    template<typename T> concept IsTag =  std::derived_from<T, Tag> && !std::is_same_v<T, EntityWrapper>;

    /**
     * @brief Concept accepting any type as a component.
     *
     * Currently a universal acceptor — any type qualifies as a component.
     * In the future this may be constrained to require specific traits
     * (e.g. trivially copyable, default-constructible, etc.).
     */
    template<typename T> concept IsComponent = requires { !IsTag<T>; };


    namespace error
    {
        inline constexpr char g_componentErrorName[] = "ComponentError: ";
        using BaseComponentError = recs::error::BaseNamedError<g_componentErrorName>;

        /// @brief Thrown when an invalid component ID is encountered.
        struct InvalidComponentId final : BaseComponentError { using BaseComponentError::BaseComponentError;};
    }

}