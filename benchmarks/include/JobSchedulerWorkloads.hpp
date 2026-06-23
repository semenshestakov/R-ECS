#pragma once
#include <benchmark/benchmark.h>
#include <cmath>
#include <cstddef>
#include <functional>
#include <vector>
#include "BenchmarkCommon.hpp"
#include "ecs/jobs/IJobScheduler.hpp"


inline double Kernel(const double v) { return std::sqrt(v * 1.000001 + 1.0) * 0.5; }


inline double Spin(const std::size_t iterations)
{
    double acc = 1.0;
    for (std::size_t i = 0; i < iterations; ++i)
        acc = Kernel(acc) + 1.0;
    return acc;
}


inline void ParallelForTransform(ecs::IJobScheduler& scheduler, benchmark::State& state, const std::size_t grain)
{
    std::vector<double> data(BENCHMARK_N, 1.0);

    for (auto _ : state)
    {
        scheduler.ParallelFor(0, BENCHMARK_N, grain, [&](const std::size_t first, const std::size_t last) {
            for (std::size_t i = first; i < last; ++i)
                data[i] = Kernel(data[i]);
        });
        benchmark::DoNotOptimize(data.data());
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * static_cast<std::int64_t>(BENCHMARK_N));
}


inline void RunWaitSingle(ecs::IJobScheduler& scheduler, benchmark::State& state)
{
    for (auto _ : state)
    {
        double acc = 0.0;
        const ecs::JobHandle handle = scheduler.Run([&] { acc = Kernel(acc + 1.0); });
        scheduler.Wait(handle);
        benchmark::DoNotOptimize(acc);
    }

    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()));
}


inline void RunWaitFanOut(ecs::IJobScheduler& scheduler, benchmark::State& state)
{
    const auto jobs = static_cast<std::size_t>(state.range(0));
    const auto work = static_cast<std::size_t>(state.range(1));
    std::vector<double> results(jobs, 0.0);
    std::vector<ecs::JobHandle> handles;
    handles.reserve(jobs);

    for (auto _ : state)
    {
        handles.clear();
        for (std::size_t i = 0; i < jobs; ++i)
            handles.push_back(scheduler.Run([&results, i, work] { results[i] = Spin(work) + static_cast<double>(i); }));

        for (const auto& handle : handles)
            scheduler.Wait(handle);

        benchmark::DoNotOptimize(results.data());
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * static_cast<std::int64_t>(jobs));
}


inline void ServicePump(ecs::IJobScheduler& scheduler, benchmark::State& state)
{
    const auto work = static_cast<std::size_t>(state.range(0));
    double sink = 0.0;
    const ecs::ServiceHandle service = scheduler.SpawnService([&] { sink += Spin(work); });

    for (auto _ : state)
    {
        scheduler.PumpServices();
        benchmark::DoNotOptimize(sink);
    }

    scheduler.StopService(service);
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()));
}
