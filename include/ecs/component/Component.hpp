#pragma once
#include <type_traits>


namespace ecs::component
{
    // Basic type aliases for memory management and component identification
    using byte = unsigned char;           ///< Fundamental byte type for raw memory operations
    using bufferSize_t = unsigned int;    ///< Type for representing buffer sizes and memory capacities
    using componentId_t = byte;           ///< Type for unique component type identifiers

    // Special value indicating an invalid or uninitialized component ID
    constexpr componentId_t INVALID_COMPONENT_ID = 0;
    constexpr componentId_t MAX_COMPONENT_ID = ~0;
    constexpr unsigned int OVERFLOW_MAX_COMPONENT_ID = static_cast<unsigned int>(MAX_COMPONENT_ID) + 1;


    /**
     * @brief Base class for all ECS components
     *
     * Provides the fundamental interface and identification mechanism
     * for all components in the Entity Component System. Each derived
     * component class must define its own unique componentId.
     */
    class BaseComponent
    {
    public:
        /// Unique identifier for the component type. Must be overridden in derived classes.
        static constexpr componentId_t componentId = INVALID_COMPONENT_ID;

        /// Virtual destructor to ensure proper cleanup of derived components
        virtual ~BaseComponent() = default;

        struct ConditionArgs {};
        /**
         * @brief Condition for component creation that always return bool
         * @param args Component constructor arguments
         * @return Always true - component will always be created
         *
         * Default creation condition that unconditionally allows component instantiation.
         */
        static bool condition(const ConditionArgs args) { return true; }

        using conditionFunction_t = bool(*)(const ConditionArgs*);
    };


    /**
     * @brief Concept constraining template parameters to BaseComponent derivatives
     *
     * Ensures type safety by restricting template arguments to classes
     * that inherit from BaseComponent, preventing invalid component types.
     */
    template<typename T> concept BaseOfComponents = std::is_base_of_v<BaseComponent, T> && T::componentId != INVALID_COMPONENT_ID;

}