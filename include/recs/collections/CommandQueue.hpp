#ifndef COMMAND_QUEUE_HPP
#define COMMAND_QUEUE_HPP
#include <functional>
#include <vector>


namespace collections
{

    /**
     * @brief A FIFO queue of deferred callable commands.
     *
     * Stores std::function<void(Args...)> entries and executes them in insertion
     * order on Flush(Args...).  The queue is swapped out before execution so that
     * commands pushed during a flush are deferred to the next flush cycle.
     *
     * @tparam Args Argument types passed to each command at flush time.
     *
     * Non-copyable (move-only) to match the ownership semantics of the
     * callables it holds.
     */
    template<typename... Args>
    class CommandQueue
    {
    public:
        typedef std::function<void(Args...)> func_t; ///< Type-erased callable.

        /**
         * @brief Constructs an empty command queue.
         */
        CommandQueue() = default;

        /**
         * @brief Default destructor.
         */
        ~CommandQueue() = default;

        CommandQueue(const CommandQueue&) = delete;
        CommandQueue& operator=(const CommandQueue&) = delete;

        /**
         * @brief Move constructor.
         */
        CommandQueue(CommandQueue&&) = default;

        /**
         * @brief Move assignment.
         * @return Reference to this queue.
         */
        CommandQueue& operator=(CommandQueue&&) = default;

        /**
         * @brief Enqueue a callable for deferred execution.
         * @tparam F Callable type (deduced)
         * @param func Callable to invoke during the next Flush()
         */
        template<typename F>
        void Push(F&& func);

        /**
         * @brief Execute all queued commands in FIFO order.
         *
         * The internal queue is swapped into a local vector so that
         * commands added during execution are not processed in the
         * same pass.
         *
         * @param args Arguments forwarded to each queued callable
         */
        void Flush(Args... args);

        /**
         * @brief Check whether the queue holds any commands.
         */
        [[nodiscard]] bool empty() const;

        /**
         * @brief Number of queued commands.
         */
        [[nodiscard]] size_t size() const;

    protected:
        std::vector<func_t> m_queue; ///< FIFO storage of pending commands.
    };

}
#include "detail/CommandQueue.ipp"

#endif

