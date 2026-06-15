#pragma once
#include <cstddef>
#include <functional>
#include "FunctionRef.hpp"
#include "JobHandle.hpp"


namespace ecs
{

    /**
     * @brief Abstract threading port: the only multithreading dependency the ECS core has.
     *
     * IJobScheduler is the seam that decouples R-ECS from any particular task
     * library. The core depends solely on this interface; concrete backends
     * (a TBB pool, an std::thread pool, a user's in-house scheduler, or the
     * built-in SerialJobScheduler) implement it in their own translation unit
     * and are injected at runtime via Registry::SetScheduler. No header under
     * include/ecs ever includes a threading library.
     *
     * The interface is intentionally coarse: every method is a per-system /
     * per-frame entry point, so the single virtual dispatch per call is
     * negligible against the work it launches. The hot, per-element work is
     * carried by the FunctionRef body and runs without further indirection.
     *
     * Implementations must be safe to call from the main thread. Whether they
     * are safe to call from worker threads is backend-defined.
     */
    class IJobScheduler
    {
    public:
        virtual ~IJobScheduler() = default;

        /// Body of a data-parallel loop, invoked over sub-ranges [first, last).
        using RangeBody = FunctionRef<void(std::size_t /*first*/, std::size_t /*last*/)>;

        /**
         * @brief Data-parallel loop over the index range [begin, end).
         *
         * The scheduler splits the range into chunks no smaller than @p grain
         * and invokes @p body on each chunk, possibly concurrently on several
         * workers. The call blocks until every chunk has completed. @p body
         * must therefore be safe to run concurrently on disjoint sub-ranges.
         *
         * @param begin Inclusive start index.
         * @param end   Exclusive end index. If end <= begin the call is a no-op.
         * @param grain Minimum number of iterations per chunk (a hint; 0 means "scheduler decides").
         * @param body  Callable applied to each sub-range. Borrowed, not stored.
         */
        virtual void ParallelFor(std::size_t begin, std::size_t end, std::size_t grain, RangeBody body) = 0;

        /**
         * @brief Schedules a single task to run asynchronously.
         * @param job Work to execute. Ownership is transferred to the scheduler.
         * @return A handle that becomes ready when the task finishes; Wait on it to join.
         * @note The serial backend runs @p job inline and returns an empty handle.
         */
        [[nodiscard]] virtual JobHandle Run(std::function<void()> job) = 0;

        /**
         * @brief Blocks until the work behind @p handle has completed.
         * @param handle A handle from Run; an empty/invalid handle returns immediately.
         */
        virtual void Wait(const JobHandle &handle) = 0;

        /**
         * @brief Number of workers the scheduler can run in parallel (>= 1).
         * @note Useful for sizing grain and partitioning work.
         */
        [[nodiscard]] virtual std::size_t WorkerCount() const = 0;
    };

} // namespace ecs
