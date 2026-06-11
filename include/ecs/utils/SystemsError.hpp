#pragma once
#include "common_recs/utils/BaseError.hpp"


namespace ecs::error
{

    inline constexpr char g_systemsErrorName[] = "SystemsError: ";
    using BaseSystemsError = recs::error::BaseNamedError<g_systemsErrorName>;

    /// @brief Thrown when a system is initialized more than once.
    struct DoubleInitialization final : BaseSystemsError { using BaseSystemsError::BaseSystemsError;};

}
