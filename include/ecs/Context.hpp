#pragma once
#include <utility>
#include "collections/Context.hpp"
#include "jobs/ThreadAffinity.hpp"


namespace ecs
{

    /**
     * @brief ECS-layer adapter over collections::Context that enforces main-thread mutation.
     *
     * collections::Context is a generic, reusable container and must not depend on
     * the ECS threading model. This adapter sits in the ECS layer (mirroring how
     * ecs::EventSystem wraps event::EventSystem) and adds a single concern: the
     * structural, mutating operations (emplace / getOrEmplace / remove) assert in
     * debug builds that they run on the main thread, just like entity and system
     * lifetime operations. Read-only access (get / has) is inherited unchanged and
     * is safe to call concurrently from worker threads on already-present entries.
     *
     * Discipline: create the shared resources you need on the main thread (e.g.
     * during Init), then have parallel systems read them with get<T>(). Use
     * getOrEmplace only on the main thread, since it may construct.
     */
    class Context final : protected collections::Context
    {
        using Super = collections::Context;
    public:
        using Super::Context;

        // Read-only access is re-exposed unchanged and stays safe to call
        // concurrently from workers on already-present entries.
        using Super::get;
        using Super::has;

        /**
         * @brief Constructs and stores a new instance of type T (main thread only).
         * @see collections::Context::emplace
         */
        template<typename T, typename... Args>
        T& emplace(Args&&... args)
        {
            ECS_ASSERT_MAIN_THREAD("Context::emplace");
            return Super::emplace<T>(std::forward<Args>(args)...);
        }

        /**
         * @brief Retrieves an existing instance or constructs one (main thread only).
         * @note Guarded unconditionally because it may construct; for concurrent
         *       read-only access from workers use get<T>() on a pre-created entry.
         * @see collections::Context::getOrEmplace
         */
        template<typename T, typename... Args>
        T& getOrEmplace(Args&&... args)
        {
            ECS_ASSERT_MAIN_THREAD("Context::getOrEmplace");
            return Super::getOrEmplace<T>(std::forward<Args>(args)...);
        }

        /**
         * @brief Removes the stored instance of type T (main thread only).
         * @see collections::Context::remove
         */
        template<typename T>
        void remove()
        {
            ECS_ASSERT_MAIN_THREAD("Context::remove");
            Super::remove<T>();
        }
    };

} // namespace ecs
