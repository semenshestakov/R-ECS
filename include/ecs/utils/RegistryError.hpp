#pragma once
#include "common_recs/utils/BaseError.hpp"


namespace ecs::error
{

    inline constexpr char g_registryErrorName[] = "RegistryError: ";
    using BaseRegistryError = recs::error::BaseNamedError<g_registryErrorName>;

    /// @brief Thrown when an invalid Entity ID is encountered.
    struct InvalidEntityId final : BaseRegistryError { using BaseRegistryError::BaseRegistryError;};

}
