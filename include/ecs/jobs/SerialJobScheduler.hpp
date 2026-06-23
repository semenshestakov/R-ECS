#pragma once
#include <algorithm>
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
        void ParallelFor(const std::size_t begin, const std::size_t end, std::size_t /*grain*/, const RangeBody body) override
        {
            if (begin < end && body)
                body(begin, end);
        }

        [[nodiscard]] JobHandle Run(const std::function<void()> job) override
        {
            if (job)
                job();
            return JobHandle{};
        }

        void Wait(const JobHandle & /*handle*/) override {}

        [[nodiscard]] std::size_t WorkerCount() const override { return 1; }

        [[nodiscard]] ServiceHandle SpawnService(std::function<void()> tick, ServiceDesc /*desc*/ = {}) override
        {
            if (!tick)
                return ServiceHandle{};

            StopSource source = StopSource::Active();
            auto state = std::make_shared<Service>(Service{source.token(), std::move(tick)});
            m_services.push_back(state);
            return ServiceHandle{std::move(source), std::move(state)};
        }

        void StopService(const ServiceHandle &handle) override
        {
            handle.request_stop(); // dropped on the next PumpServices; no thread to join
        }

        void PumpServices() override
        {
            if (m_services.empty())
                return;

            // Snapshot so a tick may spawn or stop services without invalidating iteration.
            const std::vector<std::shared_ptr<Service>> live = m_services;
            for (const auto &service : live)
                if (!service->token.stop_requested() && service->tick)
                    service->tick();

            std::erase_if(m_services, [](const std::shared_ptr<Service>& s) { return s->token.stop_requested(); });
        }

    private:
        struct Service
        {
            StopToken token;            ///< Observes the owning handle's cancellation
            std::function<void()> tick; ///< One resumable step, run once per pump
        };

        std::vector<std::shared_ptr<Service>> m_services; ///< Live cooperative services
    };

} // namespace ecs
