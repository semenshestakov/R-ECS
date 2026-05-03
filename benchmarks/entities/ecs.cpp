#include <benchmark/benchmark.h>
#include "BenchmarkClass.hpp"
#include "BenchmarkCommon.hpp"
#include "ecs/entities/EntitiesManager.hpp"
#include "ecs/entities/PrefabEntity.hpp"


using namespace ecs;


class RECSBenchmark : public benchmark::Fixture
{
protected:
    EntitiesManager manager;

    static PrefabEntity CreatePrefab()
    {
        PrefabEntity prefab;
        prefab.AddComponent<Position2d>(1.f, 2.f);
        prefab.AddComponent<Position3d>();
        return prefab;
    }
};


BENCHMARK_F(RECSBenchmark, Create_100k_Entities)(benchmark::State& state)
{
    for (auto _ : state)
    {
        EntitiesManager manager;
        PrefabEntity prefab = CreatePrefab();

        for (size_t i = 0; i < BENCHMARK_N; ++i)
        {
            manager.Create(prefab);
        }
        benchmark::DoNotOptimize(manager.size());
        state.SetItemsProcessed(BENCHMARK_N);
    }
}


BENCHMARK_F(RECSBenchmark, Access_100k_Components)(benchmark::State& state)
{

    for (auto _ : state)
    {
        state.PauseTiming();
        EntitiesManager manager;
        PrefabEntity prefab = CreatePrefab();

        std::vector<Entity> entities;
        entities.reserve(BENCHMARK_N);

        for (size_t i = 0; i < BENCHMARK_N; ++i)
        {
            entities.emplace_back(manager.Create(prefab).getEntity());
        }
        state.ResumeTiming();

        float sum = 0.f;
        for (auto& e : entities)
        {
            const auto& pos = manager.GetComponent<Position2d>(e);
            benchmark::DoNotOptimize(pos);
            sum += pos.x;
        }

        state.SetItemsProcessed(BENCHMARK_N);
        benchmark::DoNotOptimize(sum);
    }
}


BENCHMARK_F(RECSBenchmark, View_100k)(benchmark::State& state)
{

    for (auto _ : state)
    {
        state.PauseTiming();
        EntitiesManager manager;
        PrefabEntity prefab = CreatePrefab();

        for (size_t i = 0; i < BENCHMARK_N; ++i)
        {
            manager.Create(prefab);
        }
        state.ResumeTiming();

        size_t count = 0;
        for (auto [pos2d, pos3d] : manager.view<Position2d, Position3d>())
        {
            ++count;
            benchmark::DoNotOptimize(pos2d);
            benchmark::DoNotOptimize(pos3d);
        }

        state.SetItemsProcessed(BENCHMARK_N);
        benchmark::DoNotOptimize(count);
    }
}


BENCHMARK_F(RECSBenchmark, View_Zero)(benchmark::State& state)
{
    EntitiesManager manager;
    PrefabEntity prefab = CreatePrefab();

    for (size_t i = 0; i < BENCHMARK_N; ++i)
    {
        manager.Create(prefab);
    }
    {
        PrefabEntity prefab2;
        prefab2.AddComponent<Position2d>(1.f, 2.f);
        prefab2.AddComponent<Position3d>();
        prefab2.AddComponent<UnuseStruct>();
        manager.Create(prefab2);
    }

    for (auto _ : state)
    {
        size_t count = 0;
        for (auto [comp] : manager.view<UnuseStruct>())
        {
            ++count;
        }

        benchmark::DoNotOptimize(count);
    }
}


BENCHMARK_F(RECSBenchmark, Destroy_100k)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        EntitiesManager manager;
        PrefabEntity prefab = CreatePrefab();

        std::vector<EntityWrapper> entities;
        entities.reserve(BENCHMARK_N);
        for (size_t i = 0; i < BENCHMARK_N; ++i)
        {
            entities.emplace_back(manager.Create(prefab));
        }
        state.ResumeTiming();

        for (auto& e : entities)
        {
            manager.Destroy(e);
        }

        benchmark::DoNotOptimize(0);
        state.SetItemsProcessed(BENCHMARK_N);
    }
}


BENCHMARK_F(RECSBenchmark, CreateDestroy_Cycles)(benchmark::State& state)
{
    for (auto _ : state)
    {
        EntitiesManager manager;
        for (size_t c = 0; c < BENCHMARK_CYCLES; ++c)
        {
            PrefabEntity prefab = CreatePrefab();

            std::vector<EntityWrapper> entities;
            entities.reserve(BENCHMARK_M);

            for (size_t i = 0; i < BENCHMARK_M; ++i)
                entities.emplace_back(manager.Create(prefab));

            for (auto& e : entities)
                manager.Destroy(e);

            benchmark::DoNotOptimize(manager.size());
            state.SetItemsProcessed(BENCHMARK_M);
        }
    }
}