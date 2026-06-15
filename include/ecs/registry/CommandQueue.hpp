#pragma once
#include <mutex>
#include <utility>
#include <vector>
#include "collections/CommandQueue.hpp"


namespace ecs
{
    class Registry;

    /**
     * @brief ECS-specific command queue with token-gated flush.
     *
     * Wraps collections::CommandQueue<Args...> and restricts Flush() access via
     * CommandsToken so that only the Registry can trigger execution. Push() is
     * the deferral channel for systems and is safe to call from any thread,
     * including worker threads inside a parallel stage: concurrent pushes are
     * serialized by an internal mutex. Flush() (Registry-only, single-threaded)
     * holds that mutex only to swap the pending commands out into a reused
     * buffer, then runs them without the lock.
     */
    class CommandQueue final : DEEP_TEST_PROTECTED_ACCESS collections::CommandQueue<Registry&>
    {
    DEEP_TEST_PRIVATE_ACCESS:
        using Super = collections::CommandQueue<Registry&>;

    public:
        using Super::size;

        CommandQueue() = default;

        // Movable: transfer the queued commands and start with a fresh mutex.
        // Moves only happen during single-threaded setup (Registry construction),
        // never under contention, so dropping the source's lock state is safe.
        CommandQueue(CommandQueue&& other) noexcept : Super(std::move(other)) {}
        CommandQueue& operator=(CommandQueue&& other) noexcept
        {
            Super::operator=(std::move(other));
            return *this;
        }

        /**
         * @brief Enqueue a deferred command. Thread-safe; callable from any thread.
         *
         * Concurrent pushes from worker threads are serialized by an internal
         * mutex, so parallel systems can defer structural changes through here.
         * The command runs on the Registry's thread during the next Flush().
         *
         * @tparam F Callable type (deduced).
         * @param func Callable to invoke during the next Flush().
         */
        template<typename F>
        void Push(F&& func)
        {
            const std::lock_guard<std::mutex> lock(m_mutex);
            Super::Push(std::forward<F>(func));
        }

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
         *
         * Holds the mutex only long enough to swap out the pending commands (so a
         * concurrent Push() cannot race the swap), then runs them without the lock
         * — keeping user code off the lock and allowing commands to Push() again
         * without deadlock. Flush() itself is Registry-only and single-threaded.
         *
         * @param _ Authorization token (only Registry can construct)
         * @param registry Reference to the ECS Registry forwarded to each command
         */
        void Flush(CommandsToken _, Registry& registry)
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_swapQueue.swap(m_queue);
            }

            struct Guard
            {
                decltype(m_swapQueue)& guarded;
                ~Guard() { guarded.clear(); }
            } guard{m_swapQueue};

            for (auto& command : m_swapQueue)
                command(registry);

        }

    DEEP_TEST_PRIVATE_ACCESS:
        std::mutex m_mutex; ///< Serializes concurrent Push() from worker threads
        decltype(CommandQueue::m_queue) m_swapQueue;
    };

} // namespace ecs
