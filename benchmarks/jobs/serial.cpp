#include <benchmark/benchmark.h>
#include "JobSchedulerWorkloads.hpp"
#include "ecs/jobs/SerialJobScheduler.hpp"


using namespace ecs;


static void SerialJob_Header(benchmark::State& state) { for (auto _ : state) {} }
BENCHMARK(SerialJob_Header)
    ->Name("------------------------------- SerialJobScheduler -------------------------------");


static void SerialJob_ParallelFor(benchmark::State& state)
{
    SerialJobScheduler scheduler;
    ParallelForTransform(scheduler, state, 0);
}
BENCHMARK(SerialJob_ParallelFor);


static void SerialJob_ParallelFor_Grain1k(benchmark::State& state)
{
    SerialJobScheduler scheduler;
    ParallelForTransform(scheduler, state, 1'000);
}
BENCHMARK(SerialJob_ParallelFor_Grain1k);


static void SerialJob_RunWaitSingle(benchmark::State& state)
{
    SerialJobScheduler scheduler;
    RunWaitSingle(scheduler, state);
}
BENCHMARK(SerialJob_RunWaitSingle);


static void SerialJob_RunWaitFanOut(benchmark::State& state)
{
    SerialJobScheduler scheduler;
    RunWaitFanOut(scheduler, state);
}
BENCHMARK(SerialJob_RunWaitFanOut)
    ->Args({256, 0})->Args({256, 200})->Args({256, 2'000})->Args({256, 20'000})
    ->Args({1'024, 2'000})
    ->ArgNames({"jobs", "work"});


static void SerialJob_ServicePump(benchmark::State& state)
{
    SerialJobScheduler scheduler;
    ServicePump(scheduler, state);
}
BENCHMARK(SerialJob_ServicePump)->Arg(0)->Arg(2'000)->ArgNames({"work"});
