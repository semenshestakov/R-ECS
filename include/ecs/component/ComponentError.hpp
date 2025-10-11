#pragma once
#include "../utils/BaseError.hpp"


namespace ecs::component::error
{

    /**
     * @brief Base exception class for component-related errors.
     * Automatically prefixes messages with "ComponentError: ".
     */
    class BaseComponentError : public ecs::error::BaseError
    {
    public:
        template<typename... Args>
        explicit BaseComponentError(const char* message, Args&&... args) :
            BaseError(message, std::forward<Args>(args)...)
        {
            static constexpr char msgPrefix[] = "ComponentError: ";
            memmove(m_message + strlen(msgPrefix), m_message, strlen(m_message) + 1);
            memcpy(m_message, msgPrefix, strlen(msgPrefix));
        }
    };

    /// @brief Thrown when an invalid component ID is encountered.
    struct InvalidComponentId final : BaseComponentError { using BaseComponentError::BaseComponentError;};

    /// @brief Thrown when attempting to register a duplicate component.
    struct RepeatComponent final : BaseComponentError { using BaseComponentError::BaseComponentError; };

    /// @brief Thrown when component size validation fails.
    struct InvalidSizeComponents final : BaseComponentError { using BaseComponentError::BaseComponentError; };

    /// @brief Thrown when component not found
    struct InvalidComponent final : BaseComponentError { using BaseComponentError::BaseComponentError; };

}
