#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "IJobScheduler.hpp"


namespace ecs
{

    /**
     * @brief Default, dependency-free scheduler that runs everything inline.
     *
     * SerialJobScheduler executes all work synchronously on the calling thread.
     * It exists so the ECS core is fully functional with no threading backend
     * attached: a Registry always has a valid, non-null scheduler. Swap it for a
     * real pool (e.g. a TBB-backed implementation) via Registry::SetScheduler to
     * get actual parallelism without touching any system code.
     *
     * It has no worker threads, so a service cannot run a free-standing loop
     * here. Instead each spawned service is stored and ticked once per call to
     * PumpServices (which Registry::Update invokes every frame), preserving the
     * "ticks until the registry dies" semantics cooperatively on the main
     * thread. Because there are no threads to join, StopService just drops the
     * service, and destruction releases all of them — the registry's death takes
     * its services with it.
     */
    struct SerialJobScheduler final : IJobScheduler
    {
        /**
         * @brief Executes the body once over the full range on the calling thread.
         *
         * Ignores the grain hint since there is no parallelism.
         *
         * @param begin Inclusive start index.
         * @param end   Exclusive end index.
         * @param grain Ignored (serial backend).
         * @param body  Callable applied to [begin, end). No-op if empty.
         */
        void ParallelFor(std::size_t begin, std::size_t end, std::size_t grain, RangeBody body) override;

        /**
         * @brief Runs the job immediately on the calling thread.
         * @param job Work to execute. No-op if empty.
         * @return An empty JobHandle (already completed).
         */
        [[nodiscard]] JobHandle Run(std::function<void()> job) override;

        /**
         * @brief No-op; all work completes synchronously.
         * @param handle Ignored.
         */
        void Wait(const JobHandle &handle) override;

        /**
         * @brief Serial backend always reports one worker.
         * @return 1
         */
        [[nodiscard]] std::size_t WorkerCount() const override;

        /**
         * @brief Stores a service as a cooperative tickable unit.
         *
         * Since there are no worker threads, the service is not executed here.
         * PumpServices ticks it once per call.
         *
         * @param tick One resumable step of the service. No-op if empty.
         * @param desc Ignored (serial backend).
         * @return ServiceHandle used to stop the service.
         */
        [[nodiscard]] ServiceHandle SpawnService(std::function<void()> tick, ServiceDesc desc = {}) override;

        /**
         * @brief Requests a service stop (no thread to join).
         *
         * The service is dropped on the next PumpServices call.
         *
         * @param handle Handle from SpawnService.
         */
        void StopService(const ServiceHandle &handle) override;

        /**
         * @brief Ticks each live service once and removes stopped services.
         *
         * Snapshots the service list so that tick callbacks may safely spawn or
         * stop other services without invalidating iteration.
         */
        void PumpServices() override;

    private:
        /// @brief A live service tracked by the serial scheduler.
        struct Service
        {
            StopToken token;               ///< Read-only cancellation view for the tick callback
            std::function<void()> tick;    ///< One resumable step of the service
        };

        std::vector<std::shared_ptr<Service>> m_services; ///< All live services, snapshot for each PumpServices tick
    };

} // namespace ecs
