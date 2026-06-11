#ifndef ECS_COMMANDS_HPP
#define ECS_COMMANDS_HPP

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
    struct CreateEntityCmd
    {
        std::shared_ptr<PrefabEntity> prefab;
        std::function<void(Entity)> onCreated = nullptr;

        /**
         * @brief Constructs command from shared_ptr or rvalue PrefabEntity.
         * @tparam P std::shared_ptr<PrefabEntity> or PrefabEntity
         */
        template<typename P>
        explicit CreateEntityCmd(P&& p, std::function<void(Entity)> cb = nullptr) :
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
    struct DeleteEntityCmd
    {
        Entity entity;
        std::function<void(Entity)> onDeleted = nullptr;

        /**
         * @brief Executes the command — destroys the entity if alive.
         * @param registry Reference to the ECS registry
         */
        void operator()(Registry& registry) const;
    };


    /**
     * @brief Concept for a Recipe — anything with apply(PrefabEntity&).
     */
    template<typename T>
    concept Recipe = requires(T t, PrefabEntity& p) { { t.apply(p) }; };


    /**
     * @brief Flags controlling event emission for CookCmd execution.
     *
     * NONE          — no events emitted
     * PRE_EVT_CALL  — emit PrefabEvt before entity creation
     * POST_EVT_CALL — emit CreatedEntityEvt after entity creation
     */
    enum class CookFeedback : std::uint8_t
    {
        NONE = 0,
        PRE_EVT_CALL  = 1 << 0,
        POST_EVT_CALL = 1 << 1,
    };

    /**
     * @brief Deferred command that applies a Recipe and creates an entity.
     *
     * @tparam E Entity type to create (Entity or EntityWrapper-derived)
     * @tparam R Recipe type (deduced)
     *
     * When executed, the recipe is applied to populate a PrefabEntity,
     * then the entity is created. Optionally fires PrefabEvt / CreatedEntityEvt
     * based on feedback flags.
     */
    template<typename E, Recipe R>
    struct CookCmd
    {
        R recipe;                                       ///< Recipe to apply for populating entity data
        CookFeedback feedback = CookFeedback::NONE;     ///< Controls pre/post event emission

        void operator()(Registry& registry) const;
    };

}

ecs::CookFeedback operator|(ecs::CookFeedback v1, ecs::CookFeedback v2);
ecs::CookFeedback operator&(ecs::CookFeedback v1, ecs::CookFeedback v2);

#endif
#include "detail/Commands.ipp"
