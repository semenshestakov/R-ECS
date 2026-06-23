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

        /// Stops every live service and tears down the arena (no threads to join).
        ~TbbJobScheduler() override;

        TbbJobScheduler(const TbbJobScheduler&) = delete;
        TbbJobScheduler& operator=(const TbbJobScheduler&) = delete;

        void ParallelFor(std::size_t begin, std::size_t end, std::size_t grain, RangeBody body) override;

        /// @brief Schedules @p job on the arena's task group.
        /// @note The returned handle owns the join state: dropping it without
        ///       Wait still joins the task on destruction, so a fire-and-forget
        ///       handle never tears down a running task_group (which is UB).
        [[nodiscard]] JobHandle Run(std::function<void()> job) override;

        void Wait(const JobHandle& handle) override;

        [[nodiscard]] std::size_t WorkerCount() const override;

        [[nodiscard]] ServiceHandle SpawnService(std::function<void()> tick, ServiceDesc desc = {}) override;

        void StopService(const ServiceHandle& handle) override;

        /// @brief Ticks every live service once on the arena, in parallel, joining before return.
        void PumpServices() override;

        /// @brief Services run cooperatively on the shared arena, never on a dedicated OS thread.
        [[nodiscard]] bool CanHostDedicatedThreads() const noexcept override { return false; }

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace ecs
