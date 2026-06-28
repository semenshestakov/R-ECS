#pragma once
#include <cstdint>
#include <functional>
#include <mutex>
#include <typeinfo>
#include <vector>
#include "ecs/jobs/IJobScheduler.hpp"
#include "ecs/jobs/JobHandle.hpp"
#include "event/EventUtils.hpp"
#include "common_recs/utils/BaseError.hpp"

namespace event
{
    /**
     * @brief Forward declaration of the EventSystem template class
     * @tparam K The key type used to identify events
     */
    template<typename K /* key */> class EventSystem;
}


namespace ecs
{
    // Forward declarations
    class Registry;


    /**
     * @brief Type used as key for event identification in the ECS
     *
     * Events are identified by unique keys derived from their types.
     * Using std::size_t provides efficient hashing and lookup in containers.
     */
    using eventKey_t = std::size_t;

    /**
     * @brief Generates a unique compile-time key for an event type
     *
     * @tparam Event The event type to generate a key for
     * @return constexpr std::size_t A unique hash code for the event type
     *
     * @note Uses typeid(Event).hash_code() which is constexpr-friendly
     *       and provides a unique identifier per type at compile time
     *
     * @par Example:
     * @code
     * struct PlayerDiedEvent {};
     * auto key = TryGetKey<PlayerDiedEvent>(); // Unique identifier
     * @endcode
     */
    template<class Event>
    constexpr std::size_t GetEventKey()
    {
        return typeid(Event).hash_code();
    }

    /**
     * @brief ECS-specific event system class
     *
     * Specializes the generic EventSystem to use eventKey_t (std::size_t)
     * as the key type for event identification within the ECS.
     */
    class EventSystem;

    /**
     * @brief Structure containing initialization parameters for system startup.
     *
     * Provides a flexible container for passing configuration data to systems
     * during their initialization phase. The structure uses a void pointer for
     * arguments to support any configuration type, making it extensible without
     * modifying the base interface.
     */
    struct InitState
    {
        const char* nameFactory;            ///< Identifier for the systemsManager or creator of this system
        void* args;                         ///< Pointer to system-specific initialization arguments
        Registry& registry;
    };

    /**
     * @brief Initialization state for system event subscription.
     *
     * Contains required context for subscribing a system to the ECS event system,
     * including a reference to the event dispatcher and optional priority for
     * ordering event processing.
     */
    struct SubscribeState
    {
        std::reference_wrapper<EventSystem> eventSystem;       ///< Reference to the ECS event system used for subscriptions
        event::priority_t priority = event::DEFAULT_PRIORITY;  ///< Priority of the system in event processing order (higher = earlier execution)
    };

    /**
     * @brief Thread-safe sink that collects the frame-scoped jobs spawned during an update.
     *
     * Mirrors Unity's sync-point model: work scheduled while systems run is
     * recorded here and joined once at the end of SystemsManager::Update, so no
     * frame job leaks past the frame that launched it. Several systems may run in
     * parallel within a stage and push concurrently, so Add is mutex-guarded.
     * JoinAll drains in a loop, so a job that itself spawns more jobs is still
     * joined before the frame ends.
     */
    class FrameJobs
    {
    public:
        /// Records a handle to be joined at the end of the update. Callable from any worker.
        void Add(JobHandle handle)
        {
            if (!handle.valid())
                return;
            const std::lock_guard<std::mutex> lock(m_mutex);
            m_handles.push_back(std::move(handle));
        }

        /// Waits on every recorded handle (including ones spawned while joining), then clears.
        void JoinAll(IJobScheduler& scheduler)
        {
            for (;;)
            {
                std::vector<JobHandle> batch;
                {
                    const std::lock_guard<std::mutex> lock(m_mutex);
                    batch.swap(m_handles);
                }
                if (batch.empty())
                    break;
                for (const auto& handle : batch)
                    scheduler.Wait(handle);
            }
        }

    private:
        std::mutex m_mutex;                  ///< Serializes concurrent Add from parallel systems
        std::vector<JobHandle> m_handles;    ///< Frame-scoped jobs awaiting their end-of-update join
    };

    /**
     * @brief Structure containing context information for system updates.
     *
     * Passed to systems during each update cycle. It exposes the registry's job
     * scheduler so a system can spawn parallel work, and a frame-job sink so that
     * work is joined automatically at the end of the update (a Unity-style sync
     * point). Use UpdateState::Run for fire-and-forget frame work that the manager
     * joins for you; call Registry::Scheduler().Run / Wait directly when you want
     * to own the join yourself.
     */
    struct UpdateState
    {
        IJobScheduler* scheduler = nullptr; ///< Borrowed scheduler used to launch frame work (never owned).
        FrameJobs* jobs = nullptr;          ///< Borrowed sink that joins spawned work at end of Update (never owned).

        /**
         * @brief Schedules frame-scoped work and registers it for the end-of-update join.
         *
         * Equivalent to Registry::Scheduler().Run, but the returned handle is also
         * recorded in the frame-job sink, so SystemsManager::Update waits on it
         * before the frame ends. You may still Wait on the handle earlier to join
         * it yourself; the redundant end-of-frame Wait is a harmless no-op.
         *
         * @param job Work to execute asynchronously. Ownership is transferred to the scheduler.
         * @return The job handle; empty when no scheduler is attached or @p job is null.
         * @note Not [[nodiscard]]: the sink joins the work for you, so discarding the
         *       handle (fire-and-forget) is the common, intended use. Capture it only
         *       when you want to Wait early.
         */
        JobHandle Run(std::function<void()> job) const
        {
            if (!scheduler)
                return JobHandle{};

            JobHandle handle = scheduler->Run(std::move(job));
            if (jobs)
                jobs->Add(handle);
            return handle;
        }
    };

    /**
     * @typedef systemHash_t
     * @brief Type alias for system identifier hash.
     *
     * Represents a unique identifier for a system, typically generated by
     * hashing the system's type name or using a compile-time hash function.
     */
    using systemHash_t = std::size_t;

    /**
    * @brief Generates a unique compile-time hash for a system type.
    *
    * Creates a hash value that uniquely identifies the system class type.
    * Used for system lookup, dependency tracking, and registration.
    *
    * @tparam SystemCls The system class type to generate a hash for
    * @return constexpr systemHash_t A unique hash code for the system type
    *
    * @note Uses typeid(SystemCls).hash_code() which is constexpr-friendly
    *       and provides a unique identifier per type at compile time
    *
    * @par Example:
    * @code
    * class MovementSystem {};
    * auto hash = getSystemHash<MovementSystem>(); // Unique system identifier
    * @endcode
    */
    template<class SystemCls>
    constexpr systemHash_t getSystemHash()
    {
        return typeid(SystemCls).hash_code();
    }

    /**
     * @brief Defines dependencies between systems.
     *
     * Specifies a set of systems that must be executed before or after
     * a given system, ensuring proper execution order and data dependencies.
     * Used by the system scheduler to resolve execution order.
     *
     * @note This structure holds a pointer to an array of system hashes,
     *       allowing static initialization at compile time for performance.
     */
    struct DependentSystems
    {
        const systemHash_t* systemHashes = nullptr;
        std::size_t count = 0;
    };

    /**
     * @brief Classifies a dependency edge between two systems.
     *
     * The scheduler stores these flags on each edge of the dependency graph.
     * A single edge may carry several flags at once (bitwise OR).
     *
     * - Direct : a hard dependency declared with ECS_DEPENDENT_SYSTEMS. It both
     *            orders execution and propagates disabling: when a system is
     *            disabled, every system that depends on it through Direct edges
     *            (transitively) is disabled too.
     * - Data   : a soft dependency. It orders execution but never propagates
     *            disabling. Data edges are derived from declared component
     *            access (writer-before-reader) and from ECS_WEAK_DEPENDENT_SYSTEMS.
     */
    using systemDepFlags_t = std::uint8_t;
    struct SystemDep
    {
        static constexpr systemDepFlags_t None   = 0;       ///< 0b00 — no relationship
        static constexpr systemDepFlags_t Direct = 1 << 0;  ///< 0b01 — hard, orders + cascades on disable
        static constexpr systemDepFlags_t Data   = 1 << 1;  ///< 0b10 — soft, orders only
    };

    /**
     * @typedef componentHash_t
     * @brief Type alias for a compile-time component type identifier.
     */
    using componentHash_t = std::size_t;

    /**
     * @brief Generates a unique compile-time hash for a component type.
     *
     * @tparam Component The component type to generate a hash for.
     * @return componentHash_t A unique identifier for the component type.
     */
    template<class Component>
    constexpr componentHash_t getComponentHash()
    {
        return typeid(Component).hash_code();
    }

    /**
     * @brief How a system accesses a component.
     *
     * - Read      : read-only (RO)
     * - Write     : write (W)
     *
     * For scheduling, anything that writes (Write) is a writer and
     * anything that reads (Read) is a reader. The scheduler orders
     * readers after writers of the same component (writer-before-reader) and
     * orders two writers deterministically by system hash.
     */
    enum class AccessKind : std::uint8_t
    {
        Read,
        Write,
    };


    /**
     * @brief A single (component, access kind) pair declared by a system.
     */
    struct ComponentAccessEntry
    {
        componentHash_t component = 0;
        AccessKind kind = AccessKind::Read;
    };

    /**
     * @brief A view over the component access a system declares.
     *
     * Mirrors DependentSystems: a non-owning pointer plus a count, so the array
     * can live in static storage with zero per-instance overhead.
     */
    struct ComponentAccess
    {
        const ComponentAccessEntry* entries = nullptr;
        std::size_t count = 0;
    };

    /**
     * @brief Accessor tags used with the ECS_ACCESS macro.
     *
     * Wrap a component type to declare how a system touches it, e.g.
     * `ECS_ACCESS(ecs::Read<Position>, ecs::Write<Velocity>)`.
     */
    template<class Component> struct Read      { using component = Component; static constexpr AccessKind kind = AccessKind::Read; };
    template<class Component> struct Write     { using component = Component; static constexpr AccessKind kind = AccessKind::Write; };


    namespace error
    {
        inline constexpr char g_systemsErrorName[] = "SystemsError: ";
        using BaseSystemsError = recs::error::BaseNamedError<g_systemsErrorName>;

        /// @brief Thrown when a system is initialized more than once.
        struct DoubleInitialization final : BaseSystemsError { using BaseSystemsError::BaseSystemsError;};
    }


}
