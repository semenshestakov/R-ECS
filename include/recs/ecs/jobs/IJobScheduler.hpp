#pragma once
#include <cstddef>
#include <functional>
#include "FunctionRef.hpp"
#include "JobHandle.hpp"
#include "ServiceHandle.hpp"


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
     * The port covers two execution models. Frame work (ParallelFor, Run/Wait)
     * is started and joined within a frame. A service (SpawnService) is a
     * long-lived, out-of-frame unit that ticks repeatedly until it is stopped or
     * the registry that owns the scheduler dies. To stay portable down to the
     * threadless SerialJobScheduler, a service is modelled as a repeatedly-ticked
     * unit rather than a free-running loop: a real pool ticks it on a worker,
     * while a threadless backend ticks it cooperatively from PumpServices. Truly
     * dedicated OS threads (audio, blocking I/O) are an optional capability a
     * backend may advertise via CanHostDedicatedThreads, not a universal
     * contract.
     *
     * Implementations must be safe to call from the main thread. Whether they
     * are safe to call from worker threads is backend-defined.
     */
    struct IJobScheduler
    {
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

        /**
         * @brief Spawns a long-lived, out-of-frame service.
         *
         * Unlike Run (which executes @p job once), a service ticks @p tick
         * repeatedly until it is stopped via StopService or the scheduler is
         * destroyed. @p tick must be one short, resumable step, not its own
         * loop: the scheduler owns the loop and checks for cancellation between
         * ticks, which is what lets a threadless backend tick the service
         * cooperatively from PumpServices. The scheduler keeps the service alive,
         * so it dies with the registry that owns the scheduler.
         *
         * @param tick One step of the service. Ownership is transferred to the scheduler.
         * @param desc Optional scheduling/diagnostic hints; a backend may ignore them.
         * @return A handle used to stop the service; an empty tick yields an empty handle.
         */
        [[nodiscard]] virtual ServiceHandle SpawnService(std::function<void()> tick, ServiceDesc desc = {}) = 0;

        /**
         * @brief Requests a service stop and joins it.
         *
         * Sets the service's cancellation, then blocks until it has stopped (a
         * no-op join on a threadless backend, which simply drops the service on
         * its next pump). An empty/invalid handle returns immediately.
         *
         * @param handle A handle from SpawnService.
         */
        virtual void StopService(const ServiceHandle &handle) = 0;

        /**
         * @brief Advances cooperatively-ticked services once.
         *
         * The host calls this once per frame (Registry::Update does so). A
         * threadless backend ticks each live service here and drops the stopped
         * ones; a backend that runs services on real workers leaves the default
         * no-op, since its services already tick themselves.
         */
        virtual void PumpServices() {}

        /**
         * @brief Whether this backend can host services on dedicated OS threads.
         *
         * Cooperative services (SpawnService) are a universal contract every
         * backend honours. Truly dedicated threads with their own cadence (for
         * audio mixing or blocking I/O) are not: a threadless backend cannot
         * provide them. Backends that can advertise it here; the default is false.
         */
        [[nodiscard]] virtual bool CanHostDedicatedThreads() const noexcept { return false; }

        /**
         * @brief Data-parallel per-entity iteration over a chunk view, Unity ScheduleParallel-style.
         *
         * This is the parallel counterpart of a plain `for (auto e : view<...>())` loop. It mirrors
         * Unity DOTS' `EntityQuery.ScheduleParallel`: the chunk view is the query, the chunk is the
         * unit of parallelism, and the work is split as an index range over the matched chunks.
         *
         * The cross-archetype chunk iterator is forward-only, so this helper first does one cheap
         * pass to gather the matched chunks into a contiguous array — at which point the chunk
         * **count is known** and supports random access. That is exactly what lets it drive the
         * index-based `IJobScheduler::ParallelFor` (which recursively bisects [0, N) with a grain
         * and work-steals) instead of a forward-only `parallel_for_each`: better load balancing,
         * no serial feeder. The gather is O(number of chunks) — i.e. entities / chunk capacity —
         * so it is negligible against the per-entity work.
         *
         * Each task processes a run of `chunksPerTask` chunks (the grain, in chunks) and forward-
         * walks the entities inside, invoking @p body once per entity. Distinct chunks are distinct
         * allocations, so tasks on different chunks never write the same cache line.
         *
         * Because it only uses the `IJobScheduler` port and the core chunk iteration, this helper
         * is backend-agnostic: it runs truly in parallel on `TbbJobScheduler` and degrades to an
         * inline loop on `SerialJobScheduler`, with no threading-library dependency.
         *
         * @tparam ChunkRange A range whose elements are ChunkView (e.g. `EntitiesManager::chunkView<...>()`).
         * @tparam Body Callable invoked as body(*ChunkView::iterator) — a tuple of component
         *              references (or the chunk-entity index when no components are requested).
         *              Must be safe to call concurrently on disjoint entities.
         *
         * @param scheduler Backend the work is dispatched on; ParallelFor blocks until all chunks finish.
         * @param chunks The chunk view to iterate (the query result).
         * @param body Per-entity work.
         * @param chunksPerTask Chunks handed to a single task (grain); 0 lets the scheduler decide.
         *
         * @warning @p body runs on worker threads: read/write disjoint entity components freely,
         *          but route structural changes through the command queue (see the jobs guide).
         */
        template<typename ChunkRange, typename Body>
        void ParallelForEach(ChunkRange&& chunks, Body body, std::size_t chunksPerTask = 1);

    };

} // namespace ecs
#include "detail/IJobScheduler.ipp"
