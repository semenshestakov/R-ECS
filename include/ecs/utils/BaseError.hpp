#pragma once
#include <cstring>
#include <utility>

namespace ecs::error
{

    class BaseError
    {
    public:
        template<typename... Args>
        explicit BaseError(const char* message, Args&&... args)
        {
            if constexpr (sizeof...(args) > 0)
                sprintf(m_message, message, std::forward<Args>(args)...);
            else
                strcpy(m_message, message);
        }
        virtual ~BaseError() = default;

        inline const char* what() const noexcept{ return m_message; }

    protected:
        char m_message[256];
    };

}
