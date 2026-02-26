#pragma once


/**
 * @brief Macro to define factory registration names for an ECS component
 *
 * This macro should be used within a component class definition to specify
 * one or more factory names under which the component will be registered.
 * The names enable factory-based creation and lookup of the component type.
 *
 * @param ... One or more string literals representing factory names
 *
 * @note The macro:
 *       1. Declares friendship with IComponent for access to private names
 *       2. Defines ECS_REGISTRY_NAMES as a private static constexpr array
 *       3. Restores original access specifier (public) after definition
 *
 * @warning Must be used inside a component class derived from IComponent<T>
 * @warning Names must be string literals (compile-time constants)
 *
 * @see IComponent
 * @see ComponentsManagerRegistrator::RegisterComponent
 */
#define ECS_REGISTRY(...)                                                                                       \
    friend struct IComponent;                                                                                   \
    friend struct ISystem;                                                                                   \
    private:                                                                                                    \
    static constexpr std::array<std::string_view, sizeof((const char*[]){__VA_ARGS__}) / sizeof(const char*)>   \
    ECS_REGISTRY_NAMES = {__VA_ARGS__};                                                                         \
    public: