#pragma once
#include "collections/CommandQueue.hpp"


namespace ecs
{
    class Registry;
    
    /**
     * @brief ECS-specific command queue with token-gated flush.
     *
     * Wraps collections::CommandQueue and restricts Flush() access
     * via CommandsToken so that only the Registry can trigger execution.
     * Systems use Push() / empty() / size() normally.
     */
    class CommandQueue final : protected collections::CommandQueue
    {
        using Super = collections::CommandQueue;

    public:
        using Super::Push;
        using Super::empty;
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
         */
        void Flush(CommandsToken _) { Super::Flush(); }
    };

} // namespace ecs
