#pragma once
#include <atomic>
#include <memory>


namespace ecs
{

    /**
     * @brief Read-only view of a cooperative cancellation request.
     *
     * A StopToken is handed to a long-lived service so its tick loop can observe
     * when a stop has been requested and wind down. It never requests the stop
     * itself; that is the job of the owning StopSource. Multiple tokens share the
     * same underlying flag, so a single request_stop on the source is visible to
     * every token immediately. An empty (default-constructed) token never reports
     * a stop, which keeps it a harmless no-op.
     *
     * Modelled after std::stop_token but kept tiny and free of any threading
     * library so it can live next to the IJobScheduler port.
     */
    class StopToken
    {
    public:
        StopToken() = default;

        /**
         * @brief Wraps a shared stop flag produced by a StopSource.
         * @param flag Shared atomic flag, or empty for a token that never stops.
         */
        explicit StopToken(std::shared_ptr<const std::atomic_bool> flag) noexcept : m_flag(std::move(flag)) {}

        /**
         * @brief Whether a stop has been requested through the owning source.
         * @return true once request_stop has been called; false for an empty token.
         */
        [[nodiscard]] bool stopRequested() const noexcept { return m_flag && m_flag->load(std::memory_order_acquire); }

        /**
         * @brief Whether the token refers to a real stop state.
         * @return true if the token owns a shared flag; false for a default-constructed token.
         */
        [[nodiscard]] bool valid() const noexcept { return static_cast<bool>(m_flag); }

    private:
        std::shared_ptr<const std::atomic_bool> m_flag; ///< Shared stop flag, or empty for a no-op token
    };

    /**
     * @brief Write side of a cooperative cancellation channel.
     *
     * A StopSource owns the shared stop flag observed by the StopToken(s) it
     * hands out. A default-constructed source is empty and allocates nothing;
     * StopSource::Active() creates one backed by a real flag. request_stop sets
     * the flag once and is safe to call from any thread.
     *
     * Separating the source (write) from the token (read) mirrors
     * std::stop_source / std::stop_token: a service receives only a token and so
     * cannot cancel its siblings, while the scheduler and the ServiceHandle hold
     * the source and can request the stop.
     */
    class StopSource
    {
    public:
        /**
         * @brief Creates an empty source that owns no flag (request_stop is a no-op).
         */
        StopSource() = default;

        /**
         * @brief Wraps an existing shared stop flag.
         * @param flag The flag this source will set on request_stop.
         */
        explicit StopSource(std::shared_ptr<std::atomic_bool> flag) noexcept : m_flag(std::move(flag)) {}

        /**
         * @brief Creates an active source backed by a fresh, unset stop flag.
         * @return StopSource owning a newly allocated flag set to false.
         */
        [[nodiscard]] static StopSource Active() { return StopSource{std::make_shared<std::atomic_bool>(false)}; }

        /**
         * @brief Hands out a read-only token sharing this source's flag.
         * @return StopToken that observes the same flag as this source.
         */
        [[nodiscard]] StopToken token() const noexcept { return StopToken{m_flag}; }

        /**
         * @brief Requests cancellation. Idempotent and callable from any thread.
         *
         * Once called, every StopToken sharing this source's flag will report
         * stopRequested() == true.
         */
        void requestStop() const noexcept { if (m_flag) m_flag->store(true, std::memory_order_release);}

        /**
         * @brief Whether a stop has already been requested through this source.
         * @return true if request_stop has been called; false for an empty source.
         */
        [[nodiscard]] bool stopRequested() const noexcept { return m_flag && m_flag->load(std::memory_order_acquire); }

        /**
         * @brief Whether the source owns a real flag.
         * @return true if the source was created with Active() or a non-empty flag.
         */
        [[nodiscard]] bool valid() const noexcept { return static_cast<bool>(m_flag); }

    private:
        std::shared_ptr<std::atomic_bool> m_flag; ///< Shared stop flag this source sets on request_stop
    };

} // namespace ecs
