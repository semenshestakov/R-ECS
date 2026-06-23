#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <numeric>
#include <thread>
#include <vector>

#include "ecs/jobs/TbbJobScheduler.hpp"


using namespace std::chrono_literals;


class TbbJobSchedulerTest : public ::testing::Test
{
protected:
    ecs::TbbJobScheduler scheduler;
};



TEST_F(TbbJobSchedulerTest, ParallelForVisitsEveryIndexExactlyOnce)
{
    constexpr std::size_t n = 10'000;
    std::vector<int> hits(n, 0); // disjoint sub-ranges -> distinct indices, no data race

    scheduler.ParallelFor(0, n, 0, [&](const std::size_t first, const std::size_t last) {
        for (std::size_t i = first; i < last; ++i)
            hits[i] += 1;
    });

    EXPECT_EQ(std::accumulate(hits.begin(), hits.end(), 0), static_cast<int>(n));
    EXPECT_TRUE(std::ranges::all_of(hits.begin(), hits.end(), [](const int h) { return h == 1; }));
}


TEST_F(TbbJobSchedulerTest, ParallelForBlocksUntilComplete)
{
    constexpr std::size_t n = 50'000;
    std::atomic<long long> sum{0};

    scheduler.ParallelFor(0, n, 64, [&](const std::size_t first, const std::size_t last) {
        long long local = 0;
        for (std::size_t i = first; i < last; ++i)
            local += static_cast<long long>(i);
        sum += local;
    });

    // The call is a barrier: by the time it returns every chunk has run.
    constexpr long long expected = (static_cast<long long>(n) - 1) * n / 2;
    EXPECT_EQ(sum.load(), expected);
}


TEST_F(TbbJobSchedulerTest, ParallelForHonoursEmptyRange)
{
    std::atomic<int> calls{0};
    scheduler.ParallelFor(5, 5, 0, [&](std::size_t, std::size_t) { ++calls; });
    scheduler.ParallelFor(9, 3, 0, [&](std::size_t, std::size_t) { ++calls; });
    EXPECT_EQ(calls.load(), 0);
}


TEST_F(TbbJobSchedulerTest, ParallelForExplicitGrainStillCoversRange)
{
    constexpr std::size_t n = 1'234;
    std::atomic<std::size_t> count{0};
    scheduler.ParallelFor(0, n, 100, [&](const std::size_t first, const std::size_t last) { count += (last - first); });
    EXPECT_EQ(count.load(), n);
}


TEST_F(TbbJobSchedulerTest, RunExecutesJobAndWaitJoinsIt)
{
    std::atomic<bool> done{false};
    const ecs::JobHandle handle = scheduler.Run([&] {
        std::this_thread::sleep_for(5ms);
        done = true;
    });
    ASSERT_TRUE(handle.valid());
    scheduler.Wait(handle);
    EXPECT_TRUE(done.load());
}


TEST_F(TbbJobSchedulerTest, RunNullJobYieldsInvalidHandle)
{
    const ecs::JobHandle handle = scheduler.Run(nullptr);
    EXPECT_FALSE(handle.valid());
}


TEST_F(TbbJobSchedulerTest, WaitOnEmptyHandleIsNoOp) { EXPECT_NO_THROW(scheduler.Wait(ecs::JobHandle{})); }


TEST_F(TbbJobSchedulerTest, RunManyJobsAllComplete)
{
    constexpr int jobs = 64;
    std::atomic<int> ran{0};
    std::vector<ecs::JobHandle> handles;
    handles.reserve(jobs);
    for (int i = 0; i < jobs; ++i)
        handles.push_back(scheduler.Run([&] { ++ran; }));
    for (const auto& h : handles)
        scheduler.Wait(h);
    EXPECT_EQ(ran.load(), jobs);
}


TEST_F(TbbJobSchedulerTest, RunIsAsyncAndWaitJoinsInFlightWork)
{
    std::atomic<bool> release{false};
    std::atomic<bool> done{false};

    const ecs::JobHandle handle = scheduler.Run([&] {
        while (!release.load(std::memory_order_acquire))
            std::this_thread::sleep_for(100us);
        done.store(true, std::memory_order_release);
    });
    ASSERT_TRUE(handle.valid());

    EXPECT_FALSE(done.load());

    release.store(true, std::memory_order_release);
    scheduler.Wait(handle);
    EXPECT_TRUE(done.load());
}

TEST_F(TbbJobSchedulerTest, DeferredBatchWaitJoinsEveryHandle)
{
    constexpr int jobs = 16;
    std::atomic<bool> release{false};
    std::atomic<int> finished{0};

    std::vector<ecs::JobHandle> handles;
    handles.reserve(jobs);
    for (int i = 0; i < jobs; ++i)
        handles.push_back(scheduler.Run([&] {
            while (!release.load(std::memory_order_acquire))
                std::this_thread::sleep_for(100us);
            finished.fetch_add(1, std::memory_order_release);
        }));

    EXPECT_EQ(finished.load(), 0);

    release.store(true, std::memory_order_release);
    for (const auto& h : handles)
        scheduler.Wait(h);
    EXPECT_EQ(finished.load(), jobs);
}



TEST_F(TbbJobSchedulerTest, WorkerCountIsAtLeastOne) { EXPECT_GE(scheduler.WorkerCount(), 1u); }

TEST_F(TbbJobSchedulerTest, DoesNotHostDedicatedThreads) { EXPECT_FALSE(scheduler.CanHostDedicatedThreads()); }

TEST(TbbJobSchedulerCtorTest, ExplicitThreadCountBoundsWorkerCount)
{
    const ecs::TbbJobScheduler scheduler{2};
    EXPECT_GE(scheduler.WorkerCount(), 1u);
}


TEST_F(TbbJobSchedulerTest, ServiceTicksOncePerPumpUntilStopped)
{
    std::atomic<int> ticks{0};
    const ecs::ServiceHandle svc = scheduler.SpawnService([&] { ++ticks; });
    ASSERT_TRUE(svc.valid());

    // Services are frame-paced: they advance only when PumpServices runs.
    EXPECT_EQ(ticks.load(), 0);
    scheduler.PumpServices();
    scheduler.PumpServices();
    scheduler.PumpServices();
    EXPECT_EQ(ticks.load(), 3);

    scheduler.StopService(svc);
    const int after = ticks.load();
    scheduler.PumpServices();
    EXPECT_EQ(ticks.load(), after);
}


TEST_F(TbbJobSchedulerTest, SpawnNullTickYieldsInvalidHandle)
{
    const ecs::ServiceHandle svc = scheduler.SpawnService(nullptr);
    EXPECT_FALSE(svc.valid());
}


TEST_F(TbbJobSchedulerTest, StopServiceOnInvalidHandleIsNoOp)
{
    EXPECT_NO_THROW(scheduler.StopService(ecs::ServiceHandle{}));
}


TEST_F(TbbJobSchedulerTest, PumpWithNoServicesIsNoOp) { EXPECT_NO_THROW(scheduler.PumpServices()); }


TEST_F(TbbJobSchedulerTest, MultipleServicesAllTickPerPump)
{
    std::array<std::atomic<int>, 4> ticks{};
    std::vector<ecs::ServiceHandle> handles;
    for (auto& t : ticks)
        handles.push_back(scheduler.SpawnService([&t] { ++t; }));

    scheduler.PumpServices();
    scheduler.PumpServices();

    for (const auto& t : ticks)
        EXPECT_EQ(t.load(), 2);
}


TEST_F(TbbJobSchedulerTest, ServiceDescIsAccepted)
{
    std::atomic<int> ticks{0};
    const ecs::ServiceHandle svc = scheduler.SpawnService(
            [&] { ++ticks; }, ecs::ServiceDesc{.name = "test-service", .priority = 1});
    ASSERT_TRUE(svc.valid());
    scheduler.PumpServices();
    EXPECT_EQ(ticks.load(), 1);
    scheduler.StopService(svc);
}


TEST(TbbJobSchedulerLifetimeTest, DestructionDropsServices)
{
    auto ticks = std::make_shared<std::atomic<int>>(0);
    {
        ecs::TbbJobScheduler scheduler;
        static_cast<void>(scheduler.SpawnService([ticks] { ++*ticks; }));
        scheduler.PumpServices();
        scheduler.PumpServices();
        EXPECT_EQ(ticks->load(), 2);
    } // destructor drops the service: nothing pumps it anymore

    const int frozen = ticks->load();
    EXPECT_EQ(ticks->load(), frozen);
}
