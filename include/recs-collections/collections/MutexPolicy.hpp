#pragma once
#include <type_traits>
#include <mutex>


namespace collections
{

    /**
     * @brief Compile-time policy selecting whether a container performs internal locking.
     *
     * When @p Mutex is a real mutex type (e.g. std::mutex, std::recursive_mutex), the
     * policy is a std::true_type and stores an instance of it. lock_guard() returns a
     * std::lock_guard guarding it (scope-only, non-movable); unique_lock() returns a
     * std::unique_lock guarding it (movable, e.g. if a caller needs to unlock early).
     * When @p Mutex is `void` (the default), the policy is a std::false_type, no mutex
     * is stored, and both accessors return a no-op RAII guard, so a container built
     * with the default parameter pays no synchronization cost.
     *
     * @tparam Mutex The mutex type to use, or void to disable locking entirely.
     */
    template<typename Mutex = void>
    struct MutexPolicy : std::true_type
    {
    protected:
        mutable Mutex m_mutex;

        MutexPolicy() = default;
        MutexPolicy(const MutexPolicy&) = delete;
        MutexPolicy& operator=(const MutexPolicy&) = delete;

        MutexPolicy(MutexPolicy&&) noexcept {}
        MutexPolicy& operator=(MutexPolicy&&) noexcept { return *this; }

        [[nodiscard]] std::unique_lock<Mutex> unique_lock() const
        {
            return std::unique_lock<Mutex>(m_mutex);
        }

        [[nodiscard]] std::lock_guard<Mutex> lock_guard() const
        {
            return std::lock_guard<Mutex>(m_mutex);
        }
    };

    template<>
    struct MutexPolicy<void> : std::false_type
    {
    protected:
        /// No-op RAII guard used when locking is disabled (Mutex = void).
        struct NullLock
        {
            constexpr NullLock() noexcept = default;
        };

        [[nodiscard]] constexpr NullLock unique_lock() const noexcept
        {
            return NullLock{};
        }

        [[nodiscard]] constexpr NullLock lock_guard() const noexcept
        {
            return NullLock{};
        }
    };

}
