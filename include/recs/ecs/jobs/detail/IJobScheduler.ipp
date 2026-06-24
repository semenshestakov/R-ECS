#pragma once
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

#include "../IJobScheduler.hpp"


template<typename ChunkRange, typename Body>
void ecs::IJobScheduler::ParallelForEach(ChunkRange&& chunks, const Body body, const std::size_t chunksPerTask /* = 1 */)
{
    using ChunkType = std::decay_t<decltype(*chunks.begin())>;

    std::vector<ChunkType> matched;     // the query result: matched chunks, count now known
    for (auto&& chunk : chunks)
        matched.push_back(chunk);

    const std::size_t chunkCount = matched.size();
    if (chunkCount == 0)
        return;

    this->ParallelFor(
        0, chunkCount, chunksPerTask,
        [&](const std::size_t first, const std::size_t last)
        {
            for (std::size_t c = first; c < last; ++c)
            {
                for (auto&& entity : matched[c])
                    body(entity);
            }
        });
}