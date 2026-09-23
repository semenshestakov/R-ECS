#pragma once
#include <memory>
#include <utility>
#include "StopToken.hpp"


namespace ecs
{

    /**
     * @brief Static description of a long-lived service, supplied at spawn time.
     *
     * The scheduler interprets these fields; a backend is free to ignore any it
     * does not support. The serial backend ignores all of them. They exist so a
     * real backend can name the service for the profiler and place it sensibly
     * (priority / core) without the ECS core knowing anything about threads.
     */
    struct ServiceDesc
    {
        const char* name = nullptr; ///< Human-readable label for profilers/diagnostics (optional).
        int priority = 0;           ///< Backend-interpreted scheduling priority hint (0 = default).
    };

    /**
     * @brief Opaque handle to a long-lived, out-of-frame service.
     *
     * Returned by IJobScheduler::SpawnService. Unlike a JobHandle (transient,
     * frame-scoped work that is waited on once), a service ticks repeatedly until
     * it is stopped or the registry that owns the scheduler dies. The handle
     * carries the write side of the service's cancellation (a StopSource) so the
     * owner can request a graceful stop, plus opaque backend completion state
     * used by IJobScheduler::StopService to join a worker thread if the backend
     * runs one. The core never inspects the state.
     */
    class ServiceHandle
    {
    public:
        /**
         * @brief Default constructor. Creates an empty, invalid handle.
         */
        ServiceHandle() = default;

        /**
         * @brief Wraps the cancellation source and backend state for a spawned service.
         * @param source Write side of the service's cooperative cancellation.
         * @param state  Type-erased backend state (e.g. the service record or a join latch).
         */
        ServiceHandle(StopSource source, std::shared_ptr<void> state) noexcept :
            m_source(std::move(source)), m_state(std::move(state))
        {}

        /**
         * @brief Whether the handle refers to a live service.
         * @return true if backend state is present; false for a default-constructed handle.
         */
        [[nodiscard]] bool valid() const noexcept { return static_cast<bool>(m_state); }

        /**
         * @brief Requests the service stop cooperatively.
         *
         * Idempotent and safe to call from any thread. Does not block; pass the
         * handle to IJobScheduler::StopService to also join the service.
         *
         * @note On a threadless backend the service stops on the next PumpServices call.
         */
        void requestStop() const noexcept { m_source.requestStop(); }

        /**
         * @brief Accesses the cancellation source (for the owning scheduler).
         * @return Const reference to the embedded StopSource.
         */
        [[nodiscard]] const StopSource& source() const noexcept { return m_source; }

        /**
         * @brief Accesses the type-erased backend state (for the owning scheduler).
         * @return Const reference to the shared backend state, or empty.
         */
        [[nodiscard]] const std::shared_ptr<void>& state() const noexcept { return m_state; }

    private:
        StopSource m_source;                ///< Write side of the service's cooperative cancellation
        std::shared_ptr<void> m_state;      ///< Backend-owned join/completion state, or empty
    };

} // namespace ecs
