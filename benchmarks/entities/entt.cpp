#include <entt/entt.hpp>
#include <benchmark/benchmark.h>
#include "BenchmarkClass.hpp"
#include "BenchmarkCommon.hpp"


static constexpr int64_t kP2Bytes  = sizeof(Position2d);
static constexpr int64_t kP23Bytes = sizeof(Position2d) + sizeof(Position3d);


static void EnTT_Header(benchmark::State& state) { for (auto _ : state) {} }
BENCHMARK(EnTT_Header)
    ->Name("---------------------------------------- EnTT ----------------------------------------");



class EnTTBenchmark : public benchmark::Fixture
{
protected:
    entt::registry registry;

    static void CreatePrefab(entt::registry& reg, entt::entity e)
    {
        reg.emplace<Position2d>(e, 1.f, 2.f);
        reg.emplace<Position3d>(e);
    }
};


BENCHMARK_F(EnTTBenchmark, Create_100k_Entities)(benchmark::State& state)
{
    for (auto _ : state)
    {
        entt::registry registry;
        for (size_t i = 0; i < BENCHMARK_N; ++i)
        {
            auto e = registry.create();
            CreatePrefab(registry, e);
        }

        benchmark::DoNotOptimize(registry.storage<Position2d>().size());
        state.SetItemsProcessed(BENCHMARK_N);
    }
}


BENCHMARK_F(EnTTBenchmark, Access_100k_Components)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        std::vector<entt::entity> entities;
        entities.reserve(BENCHMARK_N);
        entt::registry registry;

        for (size_t i = 0; i < BENCHMARK_N; ++i)
        {
            auto e = registry.create();
            CreatePrefab(registry, e);
            entities.push_back(e);
        }
        state.ResumeTiming();

        float sum = 0.f;
        for (auto e : entities)
        {
            auto& pos = registry.get<Position2d>(e);
            benchmark::DoNotOptimize(pos);
            sum += pos.x;
        }

        state.SetItemsProcessed(BENCHMARK_N);
        benchmark::DoNotOptimize(sum);
    }
}


BENCHMARK_F(EnTTBenchmark, View_100k)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        entt::registry registry;
        for (size_t i = 0; i < BENCHMARK_N; ++i)
        {
            auto e = registry.create();
            CreatePrefab(registry, e);
        }
        state.ResumeTiming();

        size_t count = 0;
        auto view = registry.view<Position2d, Position3d>();
        for (auto e : view)
        {
            auto& pos2d = registry.get<Position2d>(e);
            auto& pos3d = registry.get<Position3d>(e);
            benchmark::DoNotOptimize(pos2d);
            benchmark::DoNotOptimize(pos3d);
            ++count;
        }

        state.SetItemsProcessed(BENCHMARK_N);
        benchmark::DoNotOptimize(count);
    }
}


BENCHMARK_F(EnTTBenchmark, View_Zero)(benchmark::State& state)
{
    entt::registry registry;

    for (size_t i = 0; i < BENCHMARK_N; ++i)
    {
        auto e = registry.create();
        CreatePrefab(registry, e);
    }
    {
        auto e = registry.create();
        CreatePrefab(registry, e);
        registry.emplace<UnuseStruct>(e);
    }

    for (auto _ : state)
    {
        size_t count = 0;
        auto view = registry.view<UnuseStruct>();
        for (auto e : view)
        {
            auto& t = view.get<UnuseStruct>(e);
            ++count;
        }

        benchmark::DoNotOptimize(count);
    }
}


BENCHMARK_F(EnTTBenchmark, Destroy_100k)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        std::vector<entt::entity> entities;
        entities.reserve(BENCHMARK_N);
        entt::registry registry;

        for (size_t i = 0; i < BENCHMARK_N; ++i)
        {
            auto e = registry.create();
            CreatePrefab(registry, e);
            entities.push_back(e);
        }
        state.ResumeTiming();

        for (auto e : entities)
            registry.destroy(e);

        benchmark::DoNotOptimize(0);
        state.SetItemsProcessed(BENCHMARK_N);
    }
}


BENCHMARK_F(EnTTBenchmark, CreateDestroy_Cycles)(benchmark::State& state)
{
    for (auto _ : state)
    {
        entt::registry registry;
        for (size_t c = 0; c < BENCHMARK_CYCLES; ++c)
        {
            std::vector<entt::entity> entities;
            entities.reserve(BENCHMARK_M);

            for (size_t i = 0; i < BENCHMARK_M; ++i)
            {
                auto e = registry.create();
                CreatePrefab(registry, e);
                entities.push_back(e);
            }

            for (auto e : entities)
                registry.destroy(e);

            benchmark::DoNotOptimize(0);
            state.SetItemsProcessed(BENCHMARK_M);
        }
        state.SetBytesProcessed(int64_t(state.iterations()) * BENCHMARK_CYCLES * BENCHMARK_M * kP23Bytes * 2);
    }
}


BENCHMARK_F(EnTTBenchmark, View_AfterFragmentation)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        entt::registry registry;
        std::vector<entt::entity> entities;
        entities.reserve(BENCHMARK_N);

        for (size_t i = 0; i < BENCHMARK_N; ++i)
        {
            auto e = registry.create();
            CreatePrefab(registry, e);
            entities.push_back(e);
        }
        for (size_t i = 0; i < entities.size(); i += 2)
            registry.destroy(entities[i]);
        state.ResumeTiming();

        float sum = 0.f;
        auto view = registry.view<Position2d, Position3d>();
        for (auto e : view)
        {
            auto& pos2d = view.get<Position2d>(e);
            benchmark::DoNotOptimize(view.get<Position3d>(e));
            sum += pos2d.x;
        }

        benchmark::DoNotOptimize(sum);
        state.SetItemsProcessed(BENCHMARK_N / 2);
    }
}


BENCHMARK_F(EnTTBenchmark, MixedArchetype_View)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        entt::registry registry;

        constexpr size_t chunk = BENCHMARK_N / 3;
        for (size_t i = 0; i < chunk; ++i)
        {
            auto e = registry.create();
            registry.emplace<Position2d>(e, 1.f, 2.f);
            registry.emplace<Position3d>(e);
        }
        for (size_t i = 0; i < chunk; ++i)
        {
            auto e = registry.create();
            registry.emplace<Position2d>(e, 3.f, 4.f);
            registry.emplace<Velocity2d>(e);
        }
        for (size_t i = 0; i < chunk; ++i)
        {
            auto e = registry.create();
            registry.emplace<Position2d>(e, 5.f, 6.f);
            registry.emplace<Position3d>(e);
            registry.emplace<Velocity2d>(e);
        }
        state.ResumeTiming();

        float sum = 0.f;
        size_t count = 0;
        auto view = registry.view<Position2d, Position3d>();
        for (auto e : view)
        {
            auto& pos2d = view.get<Position2d>(e);
            benchmark::DoNotOptimize(view.get<Position3d>(e));
            sum += pos2d.x;
            ++count;
        }

        benchmark::DoNotOptimize(sum);
        benchmark::DoNotOptimize(count);
        state.SetItemsProcessed(chunk * 2);
    }
}


BENCHMARK_F(EnTTBenchmark, Churn_ThenView)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        entt::registry registry;

        std::vector<entt::entity> wave1;
        wave1.reserve(BENCHMARK_N / 2);
        for (size_t i = 0; i < BENCHMARK_N / 2; ++i)
        {
            auto e = registry.create();
            registry.emplace<Position2d>(e, 1.f, 0.f);
            registry.emplace<Position3d>(e);
            wave1.push_back(e);
        }
        for (size_t i = 0; i < wave1.size(); i += 2)
            registry.destroy(wave1[i]);

        for (size_t i = 0; i < BENCHMARK_N / 2; ++i)
        {
            auto e = registry.create();
            registry.emplace<Position2d>(e, 2.f, 0.f);
            registry.emplace<Position3d>(e);
            registry.emplace<Health>(e);
        }
        state.ResumeTiming();

        float sum = 0.f;
        size_t count = 0;
        auto view = registry.view<Position2d, Position3d>();
        for (auto e : view)
        {
            auto& pos2d = view.get<Position2d>(e);
            benchmark::DoNotOptimize(view.get<Position3d>(e));
            sum += pos2d.x;
            ++count;
        }

        benchmark::DoNotOptimize(sum);
        benchmark::DoNotOptimize(count);
        state.SetItemsProcessed(count);
    }
}


BENCHMARK_F(EnTTBenchmark, SteadyState_CreateDestroy)(benchmark::State& state)
{
    entt::registry registry;

    {
        std::vector<entt::entity> warmup;
        warmup.reserve(BENCHMARK_M);
        for (size_t i = 0; i < BENCHMARK_M; ++i)
        {
            auto e = registry.create();
            CreatePrefab(registry, e);
            warmup.push_back(e);
        }
        for (auto e : warmup)
            registry.destroy(e);
    }

    std::vector<entt::entity> entities;
    entities.reserve(BENCHMARK_M);

    for (auto _ : state)
    {
        for (size_t i = 0; i < BENCHMARK_M; ++i)
        {
            auto e = registry.create();
            CreatePrefab(registry, e);
            entities.push_back(e);
        }
        for (auto e : entities)
            registry.destroy(e);
        entities.clear();

        benchmark::DoNotOptimize(registry.storage<Position2d>().size());
        state.SetItemsProcessed(BENCHMARK_M);
    }
}


BENCHMARK_F(EnTTBenchmark, SteadyState_CreateDestroyView)(benchmark::State& state)
{
    entt::registry registry;

    {
        std::vector<entt::entity> warmup;
        warmup.reserve(BENCHMARK_M);
        for (size_t i = 0; i < BENCHMARK_M; ++i)
        {
            auto e = registry.create();
            CreatePrefab(registry, e);
            warmup.push_back(e);
        }
        for (auto e : warmup)
            registry.destroy(e);
    }

    std::vector<entt::entity> entities;
    entities.reserve(BENCHMARK_M);

    for (auto _ : state)
    {
        for (size_t i = 0; i < BENCHMARK_M; ++i)
        {
            auto e = registry.create();
            CreatePrefab(registry, e);
            entities.push_back(e);
        }

        float sum = 0.f;
        auto view = registry.view<Position2d, Position3d>();
        for (auto e : view)
        {
            auto& pos2d = view.get<Position2d>(e);
            benchmark::DoNotOptimize(view.get<Position3d>(e));
            sum += pos2d.x;
        }
        benchmark::DoNotOptimize(sum);

        for (auto e : entities)
            registry.destroy(e);
        entities.clear();
        state.SetItemsProcessed(BENCHMARK_M);
    }
}
