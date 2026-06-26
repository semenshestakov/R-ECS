#pragma once
#include <cstddef>
#include <functional>
#include <memory>
#include "../../../recs/ecs/jobs/IJobScheduler.hpp"


namespace ecs
{

    /**
     * @brief oneTBB-backed scheduler: real parallelism for the ECS core.
     *
     * TbbJobScheduler implements the IJobScheduler port on top of Intel oneTBB.
     * Frame work runs in a private tbb::task_arena: ParallelFor maps onto
     * tbb::parallel_for, while Run/Wait drive a tbb::task_group whose join state
     * travels behind the opaque JobHandle. Install it with Registry::SetScheduler
     * to parallelise schedule stages without touching any system code.
     *
     * Services are cooperative and frame-paced, mirroring Unity's job model:
     * there is no dedicated OS thread per service. SpawnService merely records the
     * tick; PumpServices (called once per frame by Registry::Update) dispatches
     * every live service's tick onto the arena, runs them in parallel, and joins
     * before returning. The scheduler owns the loop and checks cancellation
     * between ticks, exactly as the port requires. Because no service blocks its
     * own thread, the backend reports CanHostDedicatedThreads() == false: truly
     * blocking subsystems (audio, socket I/O) are out of scope for this backend.
     *
     * Lifetime follows the port's invariant: the scheduler owns its services, so
     * destroying it (e.g. when the owning Registry dies, or on
     * SetScheduler(nullptr)) requests every service to stop and drops it; there
     * are no threads to join.
     *
     * The header pulls in no TBB symbols — every oneTBB type lives behind a PIMPL
     * in the implementation translation unit, so consumers of this header need not
     * see or link TBB through it.
     */
    class TbbJobScheduler final : public IJobScheduler
    {
    public:
        /**
         * @brief Creates the scheduler and its worker arena.
         * @param threads Number of workers the arena may use; 0 lets oneTBB choose
         *                (typically the hardware concurrency). Services share these
         *                workers; the backend spawns no threads of its own.
         */
        explicit TbbJobScheduler(std::size_t threads = 0);

        /**
         * @brief Stops every live service and tears down the arena.
         *
         * Requests every live service to stop, drops them, and destroys the
         * internal tbb::task_arena. There are no threads to join — the PIMPL
         * destructor handles all cleanup synchronously.
         *
         * @note Lifetime follows the port's invariant: the scheduler owns its
         *       services, so destroying it (e.g. when the owning Registry dies,
         *       or on SetScheduler(nullptr)) is safe and immediate.
         */
        ~TbbJobScheduler() override;

        TbbJobScheduler(const TbbJobScheduler&) = delete;
        TbbJobScheduler& operator=(const TbbJobScheduler&) = delete;

        /**
         * @brief Data-parallel loop over [begin, end) via tbb::parallel_for.
         *
         * Delegates to tbb::parallel_for with a simple_range split by @p grain.
         * The call blocks until every chunk has completed. @p body must be safe
         * to run concurrently on disjoint sub-ranges.
         *
         * @param begin Inclusive start index.
         * @param end   Exclusive end index. If end <= begin the call is a no-op.
         * @param grain Minimum number of iterations per chunk (a hint; 0 means "scheduler decides").
         * @param body  Callable applied to each sub-range. Borrowed, not stored.
         *
         * @see IJobScheduler::ParallelFor
         */
        void ParallelFor(std::size_t begin, std::size_t end, std::size_t grain, RangeBody body) override;

        /**
         * @brief Schedules a single task on the arena's tbb::task_group.
         *
         * @param job Work to execute. Ownership is transferred to the scheduler.
         * @return A handle that becomes ready when the task finishes; call Wait
         *         on it to join.
         *
         * @note The returned handle owns the join state: dropping it without
         *       Wait still joins the task on destruction, so a fire-and-forget
         *       handle never tears down a running task_group (which is UB).
         *
         * @see IJobScheduler::Run
         */
        [[nodiscard]] JobHandle Run(std::function<void()> job) override;

        /**
         * @brief Blocks until the work behind @p handle has completed.
         *
         * Joins the internal tbb::task_group for the given handle. An empty or
         * invalid handle returns immediately.
         *
         * @param handle A handle from Run.
         *
         * @see IJobScheduler::Wait
         */
        void Wait(const JobHandle& handle) override;

        /**
         * @brief Number of workers the arena can run in parallel (>= 1).
         *
         * Returns tbb::this_task_arena::max_concurrency() for the private arena.
         *
         * @return Worker count, always at least 1.
         *
         * @note Useful for sizing grain and partitioning work.
         *
         * @see IJobScheduler::WorkerCount
         */
        [[nodiscard]] std::size_t WorkerCount() const override;

        /**
         * @brief Spawns a long-lived, out-of-frame service on the arena.
         *
         * Unlike Run (which executes @p job once), a service ticks @p tick
         * repeatedly until stopped via StopService or the scheduler is destroyed.
         * @p tick must be one short, resumable step, not its own loop: the
         * scheduler owns the loop and checks for cancellation between ticks,
         * which is what lets a threadless backend tick the service cooperatively
         * from PumpServices.
         *
         * @param tick One step of the service. Ownership is transferred to the scheduler.
         * @param desc Optional scheduling/diagnostic hints; a backend may ignore them.
         * @return A handle used to stop the service; an empty tick yields an empty handle.
         *
         * @see IJobScheduler::SpawnService
         */
        [[nodiscard]] ServiceHandle SpawnService(std::function<void()> tick, ServiceDesc desc = {}) override;

        /**
         * @brief Requests a service stop and joins it.
         *
         * Sets the service's cancellation flag, then blocks until it has stopped.
         * On this threadless (cooperative) backend, the service is dropped on its
         * next pump rather than joined on a dedicated thread. An empty or invalid
         * handle returns immediately.
         *
         * @param handle A handle from SpawnService.
         *
         * @see IJobScheduler::StopService
         */
        void StopService(const ServiceHandle& handle) override;

        /**
         * @brief Ticks every live service once on the arena, in parallel.
         *
         * Dispatches every live service's tick onto the tbb::task_group, runs
         * them in parallel, and joins before returning. Called once per frame
         * by Registry::Update.
         *
         * @see IJobScheduler::PumpServices
         */
        void PumpServices() override;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl; ///< PIMPL — hides all oneTBB types (task_arena, task_group, service list) from consumers.
    };

} // namespace ecs
