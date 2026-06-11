#pragma once
#include "../CommandQueue.hpp"


namespace collections
{

    template<typename... Args>
    template<typename F>
    void CommandQueue<Args...>::Push(F&& func)
    {
        m_queue.push_back(std::forward<F>(func));
    }

    template<typename... Args>
    void CommandQueue<Args...>::Flush(Args... args)
    {
        std::vector<func_t> processing;
        processing.swap(m_queue);

        for (auto& func : processing)
            func(args...);
    }

    template<typename... Args>
    bool CommandQueue<Args...>::empty() const
    {
        return m_queue.empty();
    }

    template<typename... Args>
    size_t CommandQueue<Args...>::size() const
    {
        return m_queue.size();
    }

} // namespace collections
