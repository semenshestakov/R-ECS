#pragma once
#include <cstdio>
#include <cstring>
#include <exception>
#include <utility>


namespace recs::error
{

    /**
     * @brief Base exception class for ECS (Entity Component System) errors.
     *
     * This class provides a foundation for custom exceptions in the ECS framework.
     * It supports formatted error messages with printf-style formatting and ensures
     * safe message storage within a fixed-size buffer.
     *
     * @inherits std::exception
     */
    class BaseError : public std::exception
    {
    public:

        /**
         * @brief Constructs a BaseError with a formatted error message.
         *
         * The constructor supports both plain string messages and printf-style
         * formatted messages. The message is safely copied into an internal buffer.
         *
         * @tparam Args Variadic template parameter pack for format arguments.
         * @param message Format string or plain error message.
         * @param args Optional arguments for formatted message construction.
         *
         * @note If format arguments are provided, uses sprintf for formatting.
         *       Otherwise, uses strcpy for direct string copying.
         * @warning Total message length should not exceed 255 characters to avoid buffer truncation.
         */
        template<typename... Args>
        explicit BaseError(const char* message, Args&&... args)
        {
            if constexpr (sizeof...(args) > 0)
                snprintf(m_message, sizeof(m_message), message, std::forward<Args>(args)...);
            else
                strcpy(m_message, message);
        }
        virtual ~BaseError() = default;

        /**
         * @brief Returns the error message as a C-style string.
         *
         * @return const char* Pointer to the null-terminated error message.
         * @note The returned pointer remains valid until the exception object is destroyed.
         */
        [[nodiscard]] const char* what() const noexcept override { return m_message; }

    protected:
        /// @brief Internal buffer for storing the error message.
        char m_message[256] {};

    };

    /**
     * @brief Base exception class with a compile-time prefix name.
     *
     * Prepends a static name (e.g. "ComponentError: ") to every error message,
     * making it easy to identify which subsystem generated the error.
     *
     * @tparam NAME Compile-time string constant used as the error prefix
     *
     * @par Example:
     * @code
     * inline constexpr char g_name[] = "MyError: ";
     * using BaseMyError = BaseNamedError<g_name>;
     * struct SpecificError final : BaseMyError { using BaseMyError::BaseMyError; };
     * @endcode
     */
    template<const char NAME[]>
    class BaseNamedError : public BaseError
    {
    public:
        template<typename... Args>
        explicit BaseNamedError(const char* message, Args&&... args) :
            BaseError(message, std::forward<Args>(args)...)
        {
            memmove(m_message + strlen(NAME), m_message, strlen(m_message) + 1);
            memcpy(m_message, NAME, strlen(NAME));
        }
    };

}
