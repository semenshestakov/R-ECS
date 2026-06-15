#pragma once
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
     */
    class SerialJobScheduler final : public IJobScheduler
    {
    public:
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
    };

} // namespace ecs
