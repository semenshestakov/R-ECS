#include "ecs/jobs/TbbJobScheduler.hpp"

#include <algorithm>
#include <mutex>
#include <thread>
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
    };

    struct ServiceRec
    {
        ecs::StopSource source;
        std::thread thread;
    };

} // namespace


struct ecs::TbbJobScheduler::Impl
{
    explicit Impl(const std::size_t threads) : arena(threads ? static_cast<int>(threads) : tbb::task_arena::automatic)
    {
    }

    ~Impl()
    {
        std::vector<std::shared_ptr<ServiceRec>> live;
        {
            std::lock_guard<std::mutex> lock(mutex);
            live.swap(services);
        }

        for (const auto& s : live)
            s->source.request_stop();

        for (const auto& s : live)
        {
            if (s->thread.joinable())
                s->thread.join();
        }
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

ecs::ServiceHandle ecs::TbbJobScheduler::SpawnService(std::function<void()> tick, ServiceDesc /*desc*/)
{
    if (!tick)
        return ServiceHandle{};

    StopSource source = StopSource::Active();
    const StopToken token = source.token();

    const auto rec = std::make_shared<ServiceRec>();
    rec->source = source;
    rec->thread = std::thread(
        [token, tick = std::move(tick)] {
        while (!token.stop_requested())
            tick();
        });

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

    handle.request_stop();
    const auto rec = std::static_pointer_cast<ServiceRec>(handle.state());
    if (rec->thread.joinable())
        rec->thread.join();

    std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::erase(m_impl->services, rec);
}
