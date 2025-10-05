#pragma once
#include "../utils/BaseError.hpp"


namespace ecs::component
{

    class ComponentError final : public error::BaseError
    {
    public:
        template<typename... Args>
        explicit ComponentError(const char* message, Args&&... args) :
            error::BaseError(message, std::forward<Args>(args)...)
        {
            static constexpr char msgPrefix[] = "ComponentError: ";
            memmove(m_message + strlen(msgPrefix), m_message, strlen(m_message) + 1);
            memcpy(m_message, msgPrefix, strlen(msgPrefix));
        }
    };

}
