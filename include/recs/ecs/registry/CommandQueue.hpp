#pragma once
#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>
#include "collections/CommandQueue.hpp"
#include "ecs/jobs/IJobScheduler.hpp"


namespace ecs
{
    class Registry;

    /**
     * @brief ECS-specific command queue with token-gated flush and per-worker buffers.
     *
     * Wraps collections::CommandQueue<Args...> and restricts Flush() access via
     * CommandsToken so that only the Registry can trigger execution.
     *
     * Push() is the deferral channel for systems and is callable from any thread,
     * including worker threads inside a parallel stage. To keep the hot path off a
     * shared lock, each worker accumulates into its **own** bucket, selected by
     * IJobScheduler::WorkerIndex(): two workers never touch the same container, so
     * concurrent pushes need no synchronization at all. Only pushes from threads
     * outside the pool (kExternalWorker — chiefly the main thread between frames)
     * fall back to a single mutex-guarded bucket; that path is rare and
     * uncontended. A backend whose WorkerIndex() returns the default
     * kExternalWorker simply routes everything through the mutex bucket, exactly
     * as before — correct, just unoptimized.
     *
     * Flush() (Registry-only, single-threaded, between stages when no worker is
     * running) snapshots every bucket — worker buckets first, in index order, then
     * the external bucket — before executing any command, so commands pushed
     * *during* a flush are deferred to the next flush. The snapshot is drained
     * through a reused buffer whose capacity is retained across frames, so steady
     * state performs no per-frame allocation.
     */
    class CommandQueue final : DEEP_TEST_PROTECTED_ACCESS collections::CommandQueue<Registry&>
    {
    DEEP_TEST_PRIVATE_ACCESS:
        using Super = collections::CommandQueue<Registry&>;

    public:
        CommandQueue() = default;

        // Movable: transfer the queued commands and start with a fresh mutex.
        // Moves only happen during single-threaded setup (Registry construction),
        // never under contention, so dropping the source's lock state is safe.
        CommandQueue(CommandQueue&& other) noexcept :
            Super(std::move(other)),
            m_scheduler(other.m_scheduler),
            m_workerCount(other.m_workerCount),
            m_workerBuckets(std::move(other.m_workerBuckets)),
            m_drain(std::move(other.m_drain))
        {}

        CommandQueue& operator=(CommandQueue&& other) noexcept
        {
            Super::operator=(std::move(other));
            m_scheduler     = other.m_scheduler;
            m_workerCount   = other.m_workerCount;
            m_workerBuckets = std::move(other.m_workerBuckets);
            m_drain         = std::move(other.m_drain);
            return *this;
        }

        /**
         * @brief Binds the queue to a scheduler and sizes the per-worker buckets.
         *
         * Registry calls this from its constructor and from SetScheduler. The
         * bucket count tracks IJobScheduler::WorkerCount(). Called only on the main
         * thread during setup, when the queue is empty, so resizing never drops
         * pending commands.
         *
         * @param scheduler The active scheduler, queried for WorkerCount()/WorkerIndex().
         */
        void bindScheduler(IJobScheduler& scheduler)
        {
            m_scheduler   = &scheduler;
            m_workerCount = scheduler.WorkerCount();
            m_workerBuckets.resize(m_workerCount);
        }

        /**
         * @brief Enqueue a deferred command. Thread-safe; callable from any thread.
         *
         * A worker thread (WorkerIndex() < WorkerCount()) appends to its own bucket
         * with no lock — distinct workers own distinct buckets. Any other thread
         * (the main thread, or a backend without per-worker indices) appends to the
         * shared external bucket under a mutex. The command runs on the Registry's
         * thread during the next Flush().
         *
         * @tparam F Callable type (deduced).
         * @param func Callable to invoke during the next Flush().
         */
        template<typename F>
        void Push(F&& func)
        {
            const std::size_t worker = m_scheduler ? m_scheduler->WorkerIndex() : IJobScheduler::kExternalWorker;
            if (worker < m_workerCount)
            {
                m_workerBuckets[worker].emplace_back(std::forward<F>(func)); // lock-free: this worker owns bucket `worker`
                return;
            }

            const std::lock_guard<std::mutex> lock(m_mutex);                 // external/main thread: rare, uncontended
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
         * @brief Execute all deferred commands in a deterministic order.
         *
         * Snapshots every bucket before running anything — worker buckets in index
         * order, then the external bucket — so a command pushed during the flush
         * lands in a now-empty bucket and is deferred to the next flush (matching
         * the single-queue semantics). The snapshot reuses m_drain, whose capacity
         * survives across frames, so steady state allocates nothing. Flush() is
         * Registry-only and runs single-threaded, between stages, when no worker is
         * pushing; the external bucket is still taken under the mutex for safety.
         *
         * @param _ Authorization token (only Registry can construct).
         * @param registry Reference to the ECS Registry forwarded to each command.
         */
        void Flush(CommandsToken _, Registry& registry)
        {
            m_drain.clear();                                  // retains capacity across frames

            for (auto& bucket : m_workerBuckets)             // worker buckets first, deterministic by index
            {
                for (auto& command : bucket)
                    m_drain.push_back(std::move(command));
                bucket.clear();                              // keep capacity for next frame
            }
            {
                const std::lock_guard<std::mutex> lock(m_mutex);
                for (auto& command : m_queue)                // external/main bucket last
                    m_drain.push_back(std::move(command));
                m_queue.clear();
            }

            for (auto& command : m_drain)                    // re-pushes here land in fresh buckets → next flush
                command(registry);
            m_drain.clear();
        }

        /// @brief Total number of pending commands across all buckets.
        [[nodiscard]] std::size_t size() const
        {
            std::size_t total = m_queue.size();
            for (const auto& bucket : m_workerBuckets)
                total += bucket.size();
            return total;
        }

        /// @brief True when no command is pending in any bucket.
        [[nodiscard]] bool empty() const { return size() == 0; }

    DEEP_TEST_PRIVATE_ACCESS:
        IJobScheduler* m_scheduler = nullptr;                ///< Non-owning; source of WorkerIndex()/WorkerCount(). Rebound by Registry.
        std::size_t m_workerCount = 0;                       ///< Cached scheduler.WorkerCount(); size of m_workerBuckets.
        std::vector<std::vector<func_t>> m_workerBuckets;    ///< One lock-free bucket per worker; bucket i is owned by worker i.
        std::vector<func_t> m_drain;                         ///< Reused flush snapshot buffer; capacity retained across frames.
        std::mutex m_mutex;                                  ///< Guards the external bucket (base m_queue) for non-worker pushes.
    };

} // namespace ecs
