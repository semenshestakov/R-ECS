#pragma once
#include "../CommandQueue.hpp"


namespace collections
{

    template<typename F>
    void CommandQueue::Push(F&& func)
    {
        m_queue.push_back(std::forward<F>(func));
    }

    inline void CommandQueue::Flush()
    {
        std::vector<func_t> processing;
        processing.swap(m_queue);

        for (auto& func : processing)
            func();
    }

    inline bool CommandQueue::empty() const
    {
        return m_queue.empty();
    }

    inline size_t CommandQueue::size() const
    {
        return m_queue.size();
    }

} // namespace collections
