#include <entt/entt.hpp>
#include <benchmark/benchmark.h>
#include "BenchmarkClass.hpp"
#include "BenchmarkCommon.hpp"


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


BENCHMARK_F(EnTTBenchmark, CreateDestroy_100k)(benchmark::State& state)
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
        {
            registry.destroy(e);
        }

        benchmark::DoNotOptimize(0);
        state.SetItemsProcessed(BENCHMARK_N);
    }
}


BENCHMARK_F(EnTTBenchmark, CreateDestroy_Cycles)(benchmark::State& state)
{
    for (auto _ : state)
    {
        for (size_t c = 0; c < BENCHMARK_CYCLES; ++c)
        {
            entt::registry registry;

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
    }
}