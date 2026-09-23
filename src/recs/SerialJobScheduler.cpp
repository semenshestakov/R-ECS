#include <algorithm>
#include <memory>
#include <vector>
#include "ecs/jobs/SerialJobScheduler.hpp"


void ecs::SerialJobScheduler::ParallelFor(const std::size_t begin, const std::size_t end, std::size_t /*grain*/, const RangeBody body)
{
    if (begin < end && body)
        body(begin, end);
}

ecs::JobHandle ecs::SerialJobScheduler::Run(const std::function<void()> job)
{
    if (job)
        job();
    return JobHandle{};
}

void ecs::SerialJobScheduler::Wait(const JobHandle & /*handle*/) {}

std::size_t ecs::SerialJobScheduler::WorkerCount() const { return 1; }

ecs::ServiceHandle ecs::SerialJobScheduler::SpawnService(std::function<void()> tick, ServiceDesc /*desc*/)
{
    if (!tick)
        return ServiceHandle{};

    StopSource source = StopSource::Active();
    auto state = std::make_shared<Service>(Service{source.token(), std::move(tick)});
    m_services.push_back(state);
    return ServiceHandle{std::move(source), std::move(state)};
}

void ecs::SerialJobScheduler::StopService(const ServiceHandle &handle)
{
    handle.requestStop();
}

void ecs::SerialJobScheduler::PumpServices()
{
    if (m_services.empty())
        return;

    const std::vector<std::shared_ptr<Service>> live = m_services;
    for (const auto &service : live)
        if (!service->token.stopRequested() && service->tick)
            service->tick();

    std::erase_if(m_services, [](const std::shared_ptr<Service>& s) { return s->token.stopRequested(); });
}

