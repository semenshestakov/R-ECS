// ───────────────────────────────────────────────────────────────────────
//  R-ECS example — services (out-of-frame work) on the oneTBB backend
//
//  A service is long-lived work that outlives a single frame — here, a fake
//  asset streamer that "loads" one chunk per tick. The contract:
//
//    * SpawnService takes one short, resumable step (a tick), not a loop.
//    * The scheduler owns the loop: it ticks the service once per frame from
//      PumpServices, which Registry::Update calls for you. On TbbJobScheduler
//      the tick runs on a worker thread — frame-paced, no dedicated thread.
//    * StopService requests cooperative cancellation; the registry also stops
//      every remaining service when it dies.
//
//  A service must not touch ECS state directly, so it shares progress through a
//  plain atomic kept in the context, which a system reads with ctx().get<T>().
//
//  Build: needs the oneTBB backend (link R-ECS::back_oneTBB,
//         configure with -DRECS_BACKEND_ONETBB=ON).
//  See:   docs/en/guides/jobs.md
// ───────────────────────────────────────────────────────────────────────
#include <atomic>
#include <iostream>
#include <memory>

#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/jobs/TbbJobScheduler.hpp" // R-ECS::back_oneTBB

struct StreamProgress
{
    static constexpr int total = 5;
    std::atomic<int> loaded{0};
};

// Reports streaming progress once per frame.
struct StreamReportSystem final : ecs::ISystem<StreamReportSystem>
{
    ECS_REGISTRY("svc")

    void Update(ecs::Registry& registry, const ecs::UpdateState&) override
    {
        const StreamProgress& progress = registry.ctx().get<StreamProgress>();
        std::cout << "[frame] streamed " << progress.loaded.load() << '/' << StreamProgress::total << '\n';
    }
};

int main()
{
    ecs::Registry registry = ecs::Registry::Create("svc");

    // Install the oneTBB pool: the service tick runs on a worker, pumped once
    // per frame by Registry::Update.
    registry.SetScheduler(std::make_unique<ecs::TbbJobScheduler>());
    registry.Init();

    // Create the shared resource on the main thread, before spawning the worker.
    auto& progress = registry.ctx().getOrEmplace<StreamProgress>();

    // One resumable step: "load" a single chunk per tick, then stop advancing.
    const ecs::ServiceHandle stream = registry.Scheduler().SpawnService(
        [&progress] {
            if (progress.loaded.load() < StreamProgress::total)
                progress.loaded.fetch_add(1);
        },
        ecs::ServiceDesc{.name = "asset-streaming"});

    // Each Update pumps the service (one tick on the pool) and runs the systems.
    for (int frame = 0; frame <= StreamProgress::total; ++frame)
        registry.Update();

    registry.Scheduler().StopService(stream);
    std::cout << "streaming complete\n";
}
