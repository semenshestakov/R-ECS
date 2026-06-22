#pragma once
#include "collections/CommandQueue.hpp"


namespace ecs
{
    class Registry;
    
    /**
     * @brief ECS-specific command queue with token-gated flush.
     *
     * Wraps collections::CommandQueue<Args...> and restricts Flush() access
     * via CommandsToken so that only the Registry can trigger execution.
     * Systems use Push() / empty() / size() normally.
     *
     */
    class CommandQueue final : DEEP_TEST_PROTECTED_ACCESS collections::CommandQueue<Registry&>
    {
    DEEP_TEST_PRIVATE_ACCESS:
        using Super = collections::CommandQueue<Registry&>;

    public:
        using Super::Push;
        using Super::size;

        /**
         * @brief Restricted token used to control command flushing.
         *
         * Only Registry is allowed to construct this token,
         * ensuring that Flush() can only be called from Registry.
         */
        struct CommandsToken
        {
            friend class Registry;
            CommandsToken() = default;
        };

        /**
         * @brief Execute all deferred commands.
         * @param _ Authorization token (only Registry can construct)
         * @param registry Reference to the ECS Registry forwarded to each command
         */
        void Flush(CommandsToken _, Registry& registry) { Super::Flush(registry) ; }
    };

} // namespace ecs
