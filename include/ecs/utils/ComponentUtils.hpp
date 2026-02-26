#pragma once
#include <concepts>
#include <type_traits>


namespace ecs
{
    // Basic type aliases for memory management and component identification
    using byte = unsigned char;           ///< Fundamental byte type for raw memory operations
    using bufferSize_t = unsigned int;    ///< Type for representing buffer sizes and memory capacities
    using componentId_t = byte;           ///< Type for unique component type identifiers

    // Special value indicating an invalid or uninitialized component ID
    constexpr componentId_t INVALID_COMPONENT_ID = 0;
    constexpr componentId_t MAX_COMPONENT_ID = ~0;
    constexpr unsigned int OVERFLOW_MAX_COMPONENT_ID = static_cast<unsigned int>(MAX_COMPONENT_ID) + 1;

    /// @brief Base class for all ECS components
    template <typename ComponentCls> struct IComponent;

    /**
     * @brief Concept constraining template parameters to BaseComponent derivatives
     *
     * Ensures type safety by restricting template arguments to classes
     * that inherit from BaseComponent, preventing invalid component types.
     */
    template<typename T> concept DerivedComponent = requires { std::derived_from<T, IComponent<T>>; };

}