#include <benchmark/benchmark.h>
#include "JobSchedulerWorkloads.hpp"
#include "ecs/jobs/TbbJobScheduler.hpp"


using namespace ecs;


static void TbbJob_Header(benchmark::State& state) { for (auto _ : state) {} }
BENCHMARK(TbbJob_Header)
    ->Name("-------------------------------- TbbJobScheduler --------------------------------");


static void TbbJob_ParallelFor(benchmark::State& state)
{
    TbbJobScheduler scheduler;
    ParallelForTransform(scheduler, state, 0);
}
BENCHMARK(TbbJob_ParallelFor);


static void TbbJob_ParallelFor_Grain1k(benchmark::State& state)
{
    TbbJobScheduler scheduler;
    ParallelForTransform(scheduler, state, 1'000);
}
BENCHMARK(TbbJob_ParallelFor_Grain1k);


static void TbbJob_ParallelFor_Scaling(benchmark::State& state)
{
    TbbJobScheduler scheduler{static_cast<std::size_t>(state.range(0))};
    ParallelForTransform(scheduler, state, 1'000);
}
BENCHMARK(TbbJob_ParallelFor_Scaling)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->ArgNames({"threads"});


static void TbbJob_RunWaitSingle(benchmark::State& state)
{
    TbbJobScheduler scheduler;
    RunWaitSingle(scheduler, state);
}
BENCHMARK(TbbJob_RunWaitSingle);


static void TbbJob_RunWaitFanOut(benchmark::State& state)
{
    TbbJobScheduler scheduler;
    RunWaitFanOut(scheduler, state);
}
BENCHMARK(TbbJob_RunWaitFanOut)
    ->Args({256, 0})->Args({256, 200})->Args({256, 2'000})->Args({256, 20'000})
    ->Args({1'024, 2'000})
    ->ArgNames({"jobs", "work"});


static void TbbJob_ServicePump(benchmark::State& state)
{
    TbbJobScheduler scheduler;
    ServicePump(scheduler, state);
}
BENCHMARK(TbbJob_ServicePump)->Arg(0)->Arg(2'000)->ArgNames({"work"});
