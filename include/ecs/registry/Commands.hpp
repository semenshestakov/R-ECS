#pragma once
#include <functional>
#include <memory>
#include "ecs/entities/Entity.hpp"
#include "ecs/entities/PrefabEntity.hpp"


namespace ecs
{
    class Registry;


    /**
     * @brief Typed command for deferred entity creation.
     *
     * Queued via Registry::Commands().Push() and executed during Flush().
     * Receives Registry& at execution time so no reference needs to be stored.
     * The entity is created from the prefab, then onCreated is called
     * with the resulting Entity handle.
     */
    struct CreateEntityCommand
    {
        std::shared_ptr<PrefabEntity> prefab;
        std::function<void(Entity)> onCreated = nullptr;

        /**
         * @brief Constructs command from shared_ptr or rvalue PrefabEntity.
         * @tparam P std::shared_ptr<PrefabEntity> or PrefabEntity
         */
        template<typename P>
        explicit CreateEntityCommand(P&& p, std::function<void(Entity)> cb = nullptr) :
            prefab(prefabFrom(std::forward<P>(p))),
            onCreated(std::move(cb))
        {}

        /**
         * @brief Executes the command — creates the entity and fires callback.
         * @param registry Reference to the ECS registry
         */
        void operator()(Registry& registry) const;

    private:
        static std::shared_ptr<PrefabEntity> prefabFrom(std::shared_ptr<PrefabEntity> p) { return p; }
        static std::shared_ptr<PrefabEntity> prefabFrom(PrefabEntity&& p) { return std::make_shared<PrefabEntity>(std::move(p)); }
    };


    /**
     * @brief Typed command for deferred entity destruction.
     *
     * Queued via Registry::Commands().Push() and executed during Flush().
     * Receives Registry& at execution time so no reference needs to be stored.
     * If the entity is alive, onDeleted is called first, then the entity
     * is destroyed. If already dead, the command is silently skipped.
     */
    struct DeleteEntityCommand
    {
        Entity entity;
        std::function<void(Entity)> onDeleted = nullptr;

        /**
         * @brief Executes the command — destroys the entity if alive.
         * @param registry Reference to the ECS registry
         */
        void operator()(Registry& registry) const;
    };

} // namespace ecs
