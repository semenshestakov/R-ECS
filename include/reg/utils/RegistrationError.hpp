#pragma once
#include "common_recs/utils/BaseError.hpp"


namespace req::error
{

    inline constexpr char g_baseNameError[] = "Registration: ";
    using BaseRegistrationError = recs::error::BaseNamedError<g_baseNameError>;

    /// @brief Thrown when attempting to register a duplicate name with UNIQUE strategy.
    struct UniqueRegistrationError final : BaseRegistrationError { using BaseRegistrationError::BaseRegistrationError;};

    /// @brief Thrown when a registration entry fails to be properly destroyed.
    struct FailDestructionError final : BaseRegistrationError { using BaseRegistrationError::BaseRegistrationError;};

}
