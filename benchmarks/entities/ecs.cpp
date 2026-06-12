#include <benchmark/benchmark.h>
#include "BenchmarkClass.hpp"
#include "BenchmarkCommon.hpp"
#include "ecs/entities/EntitiesManager.hpp"
#include "ecs/entities/PrefabEntity.hpp"


using namespace ecs;

static void RECS_Header(benchmark::State& state) { for (auto _ : state) {} }
BENCHMARK(RECS_Header)
    ->Name("---------------------------------------- R-ECS ----------------------------------------");


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
            entities.emplace_back(manager.Create<Entity>(prefab));
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
        manager.Create(prefab);
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
            ++count;

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
            entities.emplace_back(manager.Create(prefab));

        state.ResumeTiming();

        for (auto& e : entities)
            manager.Destroy(e);

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


BENCHMARK_F(RECSBenchmark, View_AfterFragmentation)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        EntitiesManager manager;
        PrefabEntity prefab = CreatePrefab();

        std::vector<EntityWrapper> entities;
        entities.reserve(BENCHMARK_N);
        for (size_t i = 0; i < BENCHMARK_N; ++i)
            entities.emplace_back(manager.Create(prefab));

        for (size_t i = 0; i < entities.size(); i += 2)
            manager.Destroy(entities[i]);
        state.ResumeTiming();

        float sum = 0.f;
        for (auto [pos2d, pos3d] : manager.view<Position2d, Position3d>())
        {
            sum += pos2d.x;
            benchmark::DoNotOptimize(pos3d);
        }

        benchmark::DoNotOptimize(sum);
        state.SetItemsProcessed(BENCHMARK_N / 2);
    }
}


BENCHMARK_F(RECSBenchmark, MixedArchetype_View)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        EntitiesManager manager;

        PrefabEntity prefabA;
        prefabA.AddComponent<Position2d>(1.f, 2.f);
        prefabA.AddComponent<Position3d>();

        PrefabEntity prefabB;
        prefabB.AddComponent<Position2d>(3.f, 4.f);
        prefabB.AddComponent<Velocity2d>();

        PrefabEntity prefabC;
        prefabC.AddComponent<Position2d>(5.f, 6.f);
        prefabC.AddComponent<Position3d>();
        prefabC.AddComponent<Velocity2d>();

        constexpr size_t chunk = BENCHMARK_N / 3;
        for (size_t i = 0; i < chunk; ++i) manager.Create(prefabA);
        for (size_t i = 0; i < chunk; ++i) manager.Create(prefabB);
        for (size_t i = 0; i < chunk; ++i) manager.Create(prefabC);
        state.ResumeTiming();

        float sum = 0.f;
        size_t count = 0;
        for (auto [pos2d, pos3d] : manager.view<Position2d, Position3d>())
        {
            sum += pos2d.x;
            benchmark::DoNotOptimize(pos3d);
            ++count;
        }

        benchmark::DoNotOptimize(sum);
        benchmark::DoNotOptimize(count);
        state.SetItemsProcessed(chunk * 2);
    }
}


BENCHMARK_F(RECSBenchmark, Churn_ThenView)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        EntitiesManager manager;

        PrefabEntity prefabA;
        prefabA.AddComponent<Position2d>(1.f, 0.f);
        prefabA.AddComponent<Position3d>();

        PrefabEntity prefabB;
        prefabB.AddComponent<Position2d>(2.f, 0.f);
        prefabB.AddComponent<Position3d>();
        prefabB.AddComponent<Health>();

        std::vector<EntityWrapper> wave1;
        wave1.reserve(BENCHMARK_N / 2);
        for (size_t i = 0; i < BENCHMARK_N / 2; ++i)
            wave1.emplace_back(manager.Create(prefabA));
        for (size_t i = 0; i < wave1.size(); i += 2)
            manager.Destroy(wave1[i]);

        for (size_t i = 0; i < BENCHMARK_N / 2; ++i)
            manager.Create(prefabB);

        state.ResumeTiming();

        float sum = 0.f;
        size_t count = 0;
        for (auto [pos2d, pos3d] : manager.view<Position2d, Position3d>())
        {
            sum += pos2d.x;
            benchmark::DoNotOptimize(pos3d);
            ++count;
        }

        benchmark::DoNotOptimize(sum);
        benchmark::DoNotOptimize(count);
        state.SetItemsProcessed(count);
    }
}


BENCHMARK_F(RECSBenchmark, SteadyState_CreateDestroy)(benchmark::State& state)
{
    EntitiesManager manager;
    PrefabEntity prefab = CreatePrefab();

    {
        std::vector<Entity> warmup;
        warmup.reserve(BENCHMARK_M);
        for (size_t i = 0; i < BENCHMARK_M; ++i)
            warmup.emplace_back(manager.Create<Entity>(prefab));
        for (auto& e : warmup)
            manager.Destroy(e);
    }

    std::vector<Entity> entities;
    entities.reserve(BENCHMARK_M);

    for (auto _ : state)
    {
        for (size_t i = 0; i < BENCHMARK_M; ++i)
            entities.emplace_back(manager.Create(prefab));

        for (auto& e : entities)
            manager.Destroy(e);

        entities.clear();

        benchmark::DoNotOptimize(manager.size());
        state.SetItemsProcessed(BENCHMARK_M);
    }
}


BENCHMARK_F(RECSBenchmark, SteadyState_CreateDestroyView)(benchmark::State& state)
{
    EntitiesManager manager;
    PrefabEntity prefab = CreatePrefab();

    {
        std::vector<EntityWrapper> warmup;
        warmup.reserve(BENCHMARK_M);
        for (size_t i = 0; i < BENCHMARK_M; ++i)
            warmup.emplace_back(manager.Create(prefab));
        for (auto& e : warmup)
            manager.Destroy(e);
    }

    std::vector<EntityWrapper> entities;
    entities.reserve(BENCHMARK_M);

    for (auto _ : state)
    {
        for (size_t i = 0; i < BENCHMARK_M; ++i)
            entities.emplace_back(manager.Create(prefab));

        float sum = 0.f;
        for (auto [pos2d, pos3d] : manager.view<Position2d, Position3d>())
        {
            sum += pos2d.x;
            benchmark::DoNotOptimize(pos3d);
        }
        benchmark::DoNotOptimize(sum);

        for (auto& e : entities)
            manager.Destroy(e);

        entities.clear();
        state.SetItemsProcessed(BENCHMARK_M);
    }
}
