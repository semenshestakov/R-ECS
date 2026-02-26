#pragma once
#include "common_recs/utils/BaseError.hpp"


namespace ecs::error
{
    inline constexpr char g_systemsErrorName[] = "SystemsError: ";
    using BaseSystemsError = recs::error::BaseNamedError<g_systemsErrorName>;

    /// @brief Thrown when an invalid component ID is encountered.
    struct DoubleInitialization final : BaseSystemsError { using BaseSystemsError::BaseSystemsError;};

}
