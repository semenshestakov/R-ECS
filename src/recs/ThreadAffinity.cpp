#include "ecs/jobs/ThreadAffinity.hpp"

#include <atomic>
#include <cstddef>
#include <functional>
#include <thread>
#ifndef NDEBUG
    #include <cassert>
    #include <cstdio>
#endif


namespace
{
    /// 0 means "no main thread recorded yet". A thread whose id hashes to 0
    /// simply disables the check for itself (the safe direction: never a false
    /// positive, at worst a missed one), so no dedicated "known" flag is needed.
    std::atomic<std::size_t>& mainThreadStorage() noexcept
    {
        static std::atomic<std::size_t> id{0};
        return id;
    }

    std::size_t currentThreadHash() noexcept
    {
        return std::hash<std::thread::id>{}(std::this_thread::get_id());
    }
}

void ecs::MarkMainThread() noexcept
{
    mainThreadStorage().store(currentThreadHash(), std::memory_order_relaxed);
}

bool ecs::OnMainThread() noexcept
{
    const std::size_t main = mainThreadStorage().load(std::memory_order_relaxed);
    return main == 0 || main == currentThreadHash();
}

#ifndef NDEBUG
void ecs::AssertMainThread(const char* op) noexcept
{
    if (OnMainThread())
        return;

    std::fprintf(stderr,
        "[R-ECS] structural change (%s) must run on the main thread; "
        "defer it through the command queue from worker threads\n", op);
    assert(false && "R-ECS: structural change off the main thread");
}
#endif
