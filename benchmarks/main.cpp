#include <benchmark/benchmark.h>
#include "ecs/jobs/ThreadAffinity.hpp"


// Designate the thread that runs the benchmarks as the ECS main thread before
// any benchmark executes, so the ECS_ASSERT_MAIN_THREAD affinity checks are
// active (in debug builds).
int main(int argc, char** argv)
{
    ecs::MarkMainThread();
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
