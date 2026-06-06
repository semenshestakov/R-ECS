#include <flecs.h>
#include <benchmark/benchmark.h>
#include "BenchmarkClass.hpp"
#include "BenchmarkCommon.hpp"


class FlecsBenchmark : public benchmark::Fixture
{
protected:
    flecs::world world;

    static void CreatePrefab(flecs::entity e)
    {
        e.set<Position2d>({1.f, 2.f});
        e.set<Position3d>({});
    }
};


BENCHMARK_F(FlecsBenchmark, Create_100k_Entities)(benchmark::State& state)
{
    for (auto _ : state)
    {
        flecs::world world;

        for (size_t i = 0; i < BENCHMARK_N; ++i)
        {
            auto e = world.entity();
            CreatePrefab(e);
        }

        benchmark::DoNotOptimize(world.count<Position2d>());
        state.SetItemsProcessed(BENCHMARK_N);
    }
}


BENCHMARK_F(FlecsBenchmark, Access_100k_Components)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();

        flecs::world world;
        std::vector<flecs::entity> entities;
        entities.reserve(BENCHMARK_N);

        for (size_t i = 0; i < BENCHMARK_N; ++i)
        {
            auto e = world.entity();
            CreatePrefab(e);
            entities.push_back(e);
        }
        state.ResumeTiming();

        float sum = 0.f;
        for (auto& e : entities)
        {
            const auto& pos = e.get<Position2d>();
            benchmark::DoNotOptimize(pos);
            sum += pos.x;
        }

        benchmark::DoNotOptimize(sum);
        state.SetItemsProcessed(BENCHMARK_N);
    }
}


BENCHMARK_F(FlecsBenchmark, View_100k)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();

        flecs::world world;
        for (size_t i = 0; i < BENCHMARK_N; ++i)
        {
            auto e = world.entity();
            CreatePrefab(e);
        }
        state.ResumeTiming();

        size_t count = 0;
        auto query = world.query<Position2d, Position3d>();

        query.each([&](flecs::entity e, Position2d& p2, Position3d& p3)
        {
            benchmark::DoNotOptimize(p2);
            benchmark::DoNotOptimize(p3);
            ++count;
        });

        benchmark::DoNotOptimize(count);
        state.SetItemsProcessed(BENCHMARK_N);
    }
}


BENCHMARK_F(FlecsBenchmark, View_Zero)(benchmark::State& state)
{
    flecs::world world;

    for (size_t i = 0; i < BENCHMARK_N; ++i)
    {
        auto e = world.entity();
        CreatePrefab(e);
    }
    {
        auto e = world.entity();
        CreatePrefab(e);
        e.set<UnuseStruct>({});
    }

    for (auto _ : state)
    {
        size_t count = 0;

        auto query = world.query<UnuseStruct>();
        query.each([&](flecs::entity, UnuseStruct&)
        {
            ++count;
        });

        benchmark::DoNotOptimize(count);
    }
}


BENCHMARK_F(FlecsBenchmark, Destroy_100k)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();

        flecs::world world;
        std::vector<flecs::entity> entities;
        entities.reserve(BENCHMARK_N);

        for (size_t i = 0; i < BENCHMARK_N; ++i)
        {
            auto e = world.entity();
            CreatePrefab(e);
            entities.push_back(e);
        }

        state.ResumeTiming();

        for (auto& e : entities)
            e.destruct();

        benchmark::DoNotOptimize(0);
        state.SetItemsProcessed(BENCHMARK_N);
    }
}


BENCHMARK_F(FlecsBenchmark, CreateDestroy_Cycles)(benchmark::State& state)
{
    for (auto _ : state)
    {
        flecs::world world;
        for (size_t c = 0; c < BENCHMARK_CYCLES; ++c)
        {
            std::vector<flecs::entity> entities;
            entities.reserve(BENCHMARK_M);

            for (size_t i = 0; i < BENCHMARK_M; ++i)
            {
                auto e = world.entity();
                CreatePrefab(e);
                entities.push_back(e);
            }

            for (auto& e : entities)
                e.destruct();

            benchmark::DoNotOptimize(0);
            state.SetItemsProcessed(BENCHMARK_M);
        }
    }
}
