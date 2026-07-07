#include "ecs/jobs/TbbJobScheduler.hpp"

#include <algorithm>
#include <functional>
#include <mutex>
#include <vector>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/task_arena.h>
#include <tbb/task_group.h>

namespace
{

    struct JobState
    {
        tbb::task_group group;

        ~JobState() { group.wait(); }
    };

    struct ServiceRec
    {
        ecs::StopSource source;        ///< Cooperative cancellation for this service
        std::function<void()> tick;    ///< One resumable step, run once per PumpServices
    };

} // namespace


struct ecs::TbbJobScheduler::Impl
{
    explicit Impl(const std::size_t threads) : arena(threads ? static_cast<int>(threads) : tbb::task_arena::automatic)
    {
    }

    ~Impl()
    {
        std::lock_guard<std::mutex> lock(mutex);
        for (const auto& s : services)
            s->source.requestStop();
        services.clear();
    }

    tbb::task_arena arena;
    std::mutex mutex;
    std::vector<std::shared_ptr<ServiceRec>> services;
};

ecs::TbbJobScheduler::TbbJobScheduler(const std::size_t threads) :
    m_impl(std::make_unique<Impl>(threads))
{}

ecs::TbbJobScheduler::~TbbJobScheduler() = default;

void ecs::TbbJobScheduler::ParallelFor(
    const std::size_t begin, const std::size_t end, const std::size_t grain, const RangeBody body
    )
{
    if (begin >= end || !body)
        return;

    const std::size_t grainsize = grain ? grain : 1;
    m_impl->arena.execute([&] {
        tbb::parallel_for(
            tbb::blocked_range<std::size_t>(begin, end, grainsize),
            [&](const tbb::blocked_range<std::size_t>& r) { body(r.begin(), r.end()); }
            );
    });
}

ecs::JobHandle ecs::TbbJobScheduler::Run(std::function<void()> job)
{
    if (!job)
        return JobHandle{};

    const auto state = std::make_shared<JobState>();
    m_impl->arena.execute([&] { state->group.run(std::move(job)); });
    return JobHandle{std::static_pointer_cast<void>(state)};
}

void ecs::TbbJobScheduler::Wait(const JobHandle& handle)
{
    if (!handle.valid())
        return;

    const auto state = std::static_pointer_cast<JobState>(handle.state());
    m_impl->arena.execute([&] { state->group.wait(); });
}

std::size_t ecs::TbbJobScheduler::WorkerCount() const
{
    return static_cast<std::size_t>(std::max(1, m_impl->arena.max_concurrency()));
}

std::size_t ecs::TbbJobScheduler::WorkerIndex() const
{
    const int index = tbb::this_task_arena::current_thread_index();
    if (index < 0) // tbb::task_arena::not_initialized — caller is not running inside the arena
        return kExternalWorker;
    return static_cast<std::size_t>(index);
}

ecs::ServiceHandle ecs::TbbJobScheduler::SpawnService(std::function<void()> tick, ServiceDesc /*desc*/)
{
    if (!tick)
        return ServiceHandle{};

    StopSource source = StopSource::Active();

    const auto rec = std::make_shared<ServiceRec>();
    rec->source = source;
    rec->tick = std::move(tick);

    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        m_impl->services.push_back(rec);
    }

    return ServiceHandle{std::move(source), std::static_pointer_cast<void>(rec)};
}

void ecs::TbbJobScheduler::StopService(const ServiceHandle& handle)
{
    if (!handle.valid())
        return;

    handle.requestStop();
    const auto rec = std::static_pointer_cast<ServiceRec>(handle.state());

    std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::erase(m_impl->services, rec);
}

void ecs::TbbJobScheduler::PumpServices()
{
    std::vector<std::shared_ptr<ServiceRec>> live;
    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        live = m_impl->services; // snapshot: a tick may spawn or stop services
    }

    if (!live.empty())
    {
        m_impl->arena.execute([&] {
            tbb::task_group group;
            for (const auto& s : live)
                if (!s->source.stopRequested() && s->tick)
                    group.run([s] { s->tick(); });
            group.wait();
        });
    }

    std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::erase_if(
        m_impl->services,
        [](const std::shared_ptr<ServiceRec>& s) {
            return s->source.stopRequested();
        });
}
