// ───────────────────────────────────────────────────────────────────────
//  R-ECS example — jobs (frame work) on the oneTBB backend
//
//  Installs TbbJobScheduler, so every call below runs on a real worker pool:
//    1. Scheduler().ParallelFor   — a data-parallel reduction over an index
//                                   range (each chunk owns a disjoint slice).
//    2. state.Run(...)            — fire-and-forget frame work, joined for you
//                                   at the end of SystemsManager::Update.
//    3. Scheduler().Run + Wait    — when you need the result mid-frame and want
//                                   to own the join yourself.
//
//  The systems are identical to the serial case — only main() changes the
//  backend. Worker threads must never touch ECS state directly, so the parallel
//  work communicates through plain atomics kept in the context.
//
//  Build: needs the oneTBB backend (link R-ECS::back_oneTBB,
//         configure with -DRECS_BACKEND_ONETBB=ON).
//  See:   docs/en/guides/jobs.md
// ───────────────────────────────────────────────────────────────────────
#include <atomic>
#include <cstddef>
#include <iostream>
#include <memory>

#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/jobs/TbbJobScheduler.hpp" // R-ECS::back_oneTBB

// Thread-safe results the parallel work writes and main reads after the frame.
struct JobsResult
{
    std::atomic<long long> rangeSum{0}; // produced by ParallelFor
    std::atomic<int> bakedAssets{0};    // produced by state.Run (fire-and-forget)
};

struct WorkSystem final : ecs::ISystem<WorkSystem>
{
    ECS_REGISTRY("jobs")

    void Update(ecs::Registry& registry, const ecs::UpdateState& state) override
    {
        ecs::IJobScheduler& scheduler = registry.Scheduler();
        auto& out = registry.ctx().get<JobsResult>(); // read-only handle: safe on a worker

        // 1. Data-parallel reduction. Each chunk sums a disjoint sub-range, so
        //    the partials never race; ParallelFor blocks until every chunk runs.
        constexpr std::size_t n = 1'000'000;
        scheduler.ParallelFor(0, n, 4'096, [&](const std::size_t first, const std::size_t last) {
            long long local = 0;
            for (std::size_t i = first; i < last; ++i)
                local += static_cast<long long>(i);
            out.rangeSum += local;
        });

        // 2. Fire-and-forget frame work: runs on the pool, joined by
        //    SystemsManager at the end of Update — no manual Wait here.
        state.Run([&out] { out.bakedAssets.fetch_add(1); }); // "bake navmesh"
        state.Run([&out] { out.bakedAssets.fetch_add(1); }); // "bake light probes"

        // 3. Own the join yourself when the next line needs the result now.
        std::atomic<int> answer{0};
        const ecs::JobHandle handle = scheduler.Run([&answer] { answer = 6 * 7; });
        scheduler.Wait(handle);
        std::cout << "[run]          answer computed on the pool = " << answer.load() << '\n';
    }
};

int main()
{
    ecs::Registry registry = ecs::Registry::Create("jobs");

    // Install the oneTBB pool. The system code above is unchanged: ParallelFor,
    // Run and state.Run now execute on real worker threads.
    registry.SetScheduler(std::make_unique<ecs::TbbJobScheduler>());
    registry.Init();

    registry.ctx().emplace<JobsResult>();

    std::cout << "workers = " << registry.Scheduler().WorkerCount() << '\n';
    registry.Update(); // runs systems on the pool, then joins the frame jobs

    // The end-of-update sync point guarantees the fire-and-forget bakes are done.
    const JobsResult& out = registry.ctx().get<JobsResult>();
    std::cout << "[parallel-for] sum(0..999999) = " << out.rangeSum.load() << '\n';
    std::cout << "[state.Run]    baked assets    = " << out.bakedAssets.load() << '\n';
}
