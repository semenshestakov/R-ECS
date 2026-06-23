#pragma once
#include <memory>
#include <utility>


namespace ecs
{

    /**
     * @brief Opaque, backend-agnostic handle to in-flight asynchronous work.
     *
     * A JobHandle is a value type returned by IJobScheduler::Run and waited on
     * with IJobScheduler::Wait. The ECS core never inspects its contents: the
     * concrete scheduler stuffs whatever it needs (a task group, a future, a
     * latch, ...) behind the type-erased shared state, and casts it back inside
     * its own Wait. The serial backend leaves the state empty because work has
     * already finished by the time Run returns.
     *
     * Keeping the handle a plain value (rather than a polymorphic object) lets
     * it be copied, combined, and stored cheaply, matching the ergonomics of
     * Unity's JobHandle while staying decoupled from any threading library.
     */
    class JobHandle
    {
    public:
        JobHandle() = default;

        /**
         * @brief Wraps backend-specific completion state.
         * @param state Type-erased state owned by the scheduler that produced this handle.
         */
        explicit JobHandle(std::shared_ptr<void> state) noexcept : m_state(std::move(state)) {}

        /**
         * @brief Whether the handle refers to schedulable work.
         * @note An empty handle (e.g. from the serial backend) is a valid no-op to Wait on.
         */
        [[nodiscard]] bool valid() const noexcept { return static_cast<bool>(m_state); }

        /**
         * @brief Accesses the type-erased backend state (for the owning scheduler).
         */
        [[nodiscard]] const std::shared_ptr<void>& state() const noexcept { return m_state; }

    private:
        std::shared_ptr<void> m_state; ///< Backend-owned completion state, or empty
    };

} // namespace ecs
