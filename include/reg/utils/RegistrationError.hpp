#pragma once
#include "common_recs/utils/BaseError.hpp"


namespace req::error
{

    inline constexpr char g_baseNameError[] = "Registration: ";
    using BaseRegistrationError = recs::error::BaseNamedError<g_baseNameError>;


    struct UniqueRegistrationError final : BaseRegistrationError { using BaseRegistrationError::BaseRegistrationError;};

    struct FailDestructionError final : BaseRegistrationError { using BaseRegistrationError::BaseRegistrationError;};

}
