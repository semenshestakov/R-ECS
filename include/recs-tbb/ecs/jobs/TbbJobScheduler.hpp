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
     * Services are deliberately *not* run inside the arena. A long-lived tick loop
     * pinned to an arena worker would permanently occupy a slot and starve frame
     * work, so each service instead owns a dedicated std::thread that runs
     * "while not stopped: tick()". This keeps the cooperative tick contract (the
     * scheduler owns the loop and checks cancellation between ticks) while leaving
     * the compute arena free, and lets the backend advertise
     * CanHostDedicatedThreads() == true.
     *
     * Lifetime follows the port's invariant: the scheduler owns its services, so
     * destroying it (e.g. when the owning Registry dies, or on
     * SetScheduler(nullptr)) requests every service to stop and joins its thread.
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
         *                (typically the hardware concurrency). Dedicated service
         *                threads are created on demand and are not counted here.
         */
        explicit TbbJobScheduler(std::size_t threads = 0);

        /// Stops and joins every live service, then tears down the arena.
        ~TbbJobScheduler() override;

        TbbJobScheduler(const TbbJobScheduler&) = delete;
        TbbJobScheduler& operator=(const TbbJobScheduler&) = delete;

        void ParallelFor(std::size_t begin, std::size_t end, std::size_t grain, RangeBody body) override;

        [[nodiscard]] JobHandle Run(std::function<void()> job) override;

        void Wait(const JobHandle& handle) override;

        [[nodiscard]] std::size_t WorkerCount() const override;

        [[nodiscard]] ServiceHandle SpawnService(std::function<void()> tick, ServiceDesc desc = {}) override;

        void StopService(const ServiceHandle& handle) override;

        /// @brief This backend hosts services on dedicated OS threads.
        [[nodiscard]] bool CanHostDedicatedThreads() const noexcept override { return true; }

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace ecs
