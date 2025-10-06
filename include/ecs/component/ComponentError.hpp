#pragma once
#include "../utils/BaseError.hpp"


namespace ecs::component::error
{

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

    struct InvalidComponentId final : BaseComponentError { using BaseComponentError::BaseComponentError;};
    struct RepeatComponent final : BaseComponentError { using BaseComponentError::BaseComponentError; };
    struct InvalidSizeComponents final : BaseComponentError { using BaseComponentError::BaseComponentError; };

}
