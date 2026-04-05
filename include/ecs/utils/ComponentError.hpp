#pragma once
#include "common_recs/utils/BaseError.hpp"


namespace ecs::error
{

    inline constexpr char g_componentErrorName[] = "ComponentError: ";
    using BaseComponentError = recs::error::BaseNamedError<g_componentErrorName>;

    /// @brief Thrown when an invalid component ID is encountered.
    struct InvalidComponentId final : BaseComponentError { using BaseComponentError::BaseComponentError;};

    /// @brief Thrown when attempting to register a duplicate component.
    struct RepeatComponent final : BaseComponentError { using BaseComponentError::BaseComponentError; };

    /// @brief Thrown when component size validation fails.
    struct InvalidSizeComponents final : BaseComponentError { using BaseComponentError::BaseComponentError; };

    /// @brief Thrown when component not found
    struct InvalidComponent final : BaseComponentError { using BaseComponentError::BaseComponentError; };

}
