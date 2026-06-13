#ifndef ECS_COMMANDS_HPP
#define ECS_COMMANDS_HPP

#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
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
     * @brief Typed command for deferred component addition.
     *
     * Queued via Registry::Commands().Push() and executed during Flush().
     * The component values are owned by the command, stored in a std::tuple,
     * and moved into EntitiesManager::AddComponents when the command runs.
     * If the entity is no longer alive at execution time the operation is a no-op.
     *
     * @tparam Args Component value types stored by the command.
     *
     * @note Components must be copy-constructible to be stored in the command queue
     *       (std::function requires a copyable target).
     * @note Class template argument deduction is supported, e.g.
     *       registry.Commands().Push(AddComponentsCmd{entity, Velocity{1.f}, Health{10}});
     */
    template<typename... Args>
    struct AddComponentsCmd
    {
        static_assert(sizeof...(Args) > 0, "AddComponentsCmd requires at least one component");

        Entity entity;                              ///< Entity to add components to.
        mutable std::tuple<Args...> components;     ///< Owned component values, moved out on execution.

        /**
         * @brief Constructs the command, taking ownership of the component values.
         * @tparam CArgs Forwarded component argument types (deduced)
         * @param e Entity to modify
         * @param args Component values to store and later add
         */
        template<typename... CArgs>
        explicit AddComponentsCmd(const Entity& e, CArgs&&... args) :
            entity(e),
            components(std::forward<CArgs>(args)...)
        {}

        /**
         * @brief Executes the command — moves the stored components into the entity.
         * @param registry Reference to the ECS registry
         */
        void operator()(Registry& registry) const;
    };

    template<typename... CArgs>
    AddComponentsCmd(const Entity&, CArgs&&...) -> AddComponentsCmd<std::remove_cvref_t<CArgs>...>;


    /**
     * @brief Typed command for deferred component removal.
     *
     * Queued via Registry::Commands().Push() and executed during Flush().
     * The component types to drop are template parameters (no values are stored).
     * If the entity is no longer alive at execution time the operation is a no-op.
     *
     * @tparam Args Component types to remove.
     *
     * @note Usage: registry.Commands().Push(RemoveComponentsCmd<Velocity, Health>{entity});
     */
    template<IsComponent... Args>
    struct RemoveComponentsCmd
    {
        static_assert(sizeof...(Args) > 0, "RemoveComponentsCmd requires at least one component");

        Entity entity;      ///< Entity to remove components from.

        /**
         * @brief Executes the command — removes the components from the entity.
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
