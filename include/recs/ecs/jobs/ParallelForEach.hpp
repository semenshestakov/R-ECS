#pragma once
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

#include "IJobScheduler.hpp"


namespace ecs
{

    /**
     * @brief Data-parallel per-entity iteration over a chunk view, Unity ScheduleParallel-style.
     *
     * This is the parallel counterpart of a plain `for (auto e : view<...>())` loop. It mirrors
     * Unity DOTS' `EntityQuery.ScheduleParallel`: the chunk view is the query, the chunk is the
     * unit of parallelism, and the work is split as an index range over the matched chunks.
     *
     * The cross-archetype chunk iterator is forward-only, so this helper first does one cheap
     * pass to gather the matched chunks into a contiguous array — at which point the chunk
     * **count is known** and supports random access. That is exactly what lets it drive the
     * index-based `IJobScheduler::ParallelFor` (which recursively bisects [0, N) with a grain
     * and work-steals) instead of a forward-only `parallel_for_each`: better load balancing,
     * no serial feeder. The gather is O(number of chunks) — i.e. entities / chunk capacity —
     * so it is negligible against the per-entity work.
     *
     * Each task processes a run of `chunksPerTask` chunks (the grain, in chunks) and forward-
     * walks the entities inside, invoking @p body once per entity. Distinct chunks are distinct
     * allocations, so tasks on different chunks never write the same cache line.
     *
     * Because it only uses the `IJobScheduler` port and the core chunk iteration, this helper
     * is backend-agnostic: it runs truly in parallel on `TbbJobScheduler` and degrades to an
     * inline loop on `SerialJobScheduler`, with no threading-library dependency.
     *
     * @tparam ChunkRange A range whose elements are ChunkView (e.g. `EntitiesManager::chunkView<...>()`).
     * @tparam Body Callable invoked as body(*ChunkView::iterator) — a tuple of component
     *              references (or the chunk-entity index when no components are requested).
     *              Must be safe to call concurrently on disjoint entities.
     *
     * @param scheduler Backend the work is dispatched on; ParallelFor blocks until all chunks finish.
     * @param chunks The chunk view to iterate (the query result).
     * @param body Per-entity work.
     * @param chunksPerTask Chunks handed to a single task (grain); 0 lets the scheduler decide.
     *
     * @warning @p body runs on worker threads: read/write disjoint entity components freely,
     *          but route structural changes through the command queue (see the jobs guide).
     */
    template<typename ChunkRange, typename Body>
    void ParallelForEach(IJobScheduler& scheduler, ChunkRange&& chunks, Body body, std::size_t chunksPerTask = 1)
    {
        using ChunkType = std::decay_t<decltype(*chunks.begin())>;

        std::vector<ChunkType> matched;     // the query result: matched chunks, count now known
        for (auto&& chunk : chunks)
            matched.push_back(chunk);

        const std::size_t chunkCount = matched.size();
        if (chunkCount == 0)
            return;

        scheduler.ParallelFor(
            0, chunkCount, chunksPerTask,
            [&](const std::size_t first, const std::size_t last) {
                for (std::size_t c = first; c < last; ++c)
                    for (auto&& entity : matched[c])
                        body(entity);
            });
    }

} // namespace ecs
