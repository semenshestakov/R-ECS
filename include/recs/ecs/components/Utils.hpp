#pragma once
#include <cstdint>
#include "common_recs/utils/BaseError.hpp"


namespace ecs
{
    // Basic type aliases for memory management and component identification
    using byte = std::byte;               ///< Fundamental byte type for raw memory operations
    using bufferSize_t = unsigned int;    ///< Type for representing buffer sizes and memory capacities
    using componentId_t = std::uint32_t;  ///< Type for unique component type identifiers

    // Special value indicating an invalid or uninitialized component ID
    constexpr componentId_t INVALID_COMPONENT_ID = 0;
    constexpr componentId_t MAX_COMPONENT_ID = ~0;
    constexpr unsigned int OVERFLOW_MAX_COMPONENT_ID = static_cast<unsigned int>(MAX_COMPONENT_ID) + 1;

    /**
     * @brief Concept accepting any type as a component.
     *
     * Currently a universal acceptor — any type qualifies as a component.
     * In the future this may be constrained to require specific traits
     * (e.g. trivially copyable, default-constructible, etc.).
     */
    template<typename T> concept IsComponent = requires { true; };


    namespace error
    {
        inline constexpr char g_componentErrorName[] = "ComponentError: ";
        using BaseComponentError = recs::error::BaseNamedError<g_componentErrorName>;

        /// @brief Thrown when an invalid component ID is encountered.
        struct InvalidComponentId final : BaseComponentError { using BaseComponentError::BaseComponentError;};
    }

}