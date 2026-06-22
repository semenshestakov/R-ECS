// ───────────────────────────────────────────────────────────────────────
//  R-ECS example — recipes, cooking & prefab events
//
//  A recipe describes how to fill a prefab. CookCmd builds the entity at
//  flush time and can fire PrefabEvt (before) / CreatedEntityEvt (after) so
//  systems can enrich the prefab or react to the new entity.
//
//  See: docs/en/guides/recipes-and-cooking.md
// ───────────────────────────────────────────────────────────────────────
#include <cassert>
#include <iostream>

#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/registry/Commands.hpp"
#include "ecs/registry/Events.hpp"

struct Health     { float value = 100.f; };
struct Position2d { float x{}, y{}; };
struct Tag        { unsigned id = 0; };

// ── A typed wrapper (no data members beyond EntityWrapper) ──────────────
struct Player final : ecs::EntityWrapper
{
    using EntityWrapper::EntityWrapper;
    float GetHealth() const { return GetComponent<Health>().value; }
};

// ── A recipe: anything with apply(PrefabEntity&) ────────────────────────
struct PlayerRecipe
{
    static void apply(ecs::PrefabEntity& p)
    {
        p.AddComponent<Health>(100.f);
        p.AddComponent<Position2d>(0.f, 0.f);
    }
};

// ── System that enriches the prefab before the entity exists ────────────
struct PlayerEnricher final : ecs::ISystem<PlayerEnricher>
{
    ECS_REGISTRY("game")
    void OnPrefab(ecs::Registry&, const ecs::PrefabEvt<Player>& e)
    {
        e.prefab.AddComponent<Tag>(99);
        std::cout << "[Enricher] tagged player prefab\n";
    }
    ECS_EVENT(OnPrefab, ecs::PrefabEvt<Player>)
};

// ── System that reacts to the freshly created entity ────────────────────
struct PlayerSpawnLogger final : ecs::ISystem<PlayerSpawnLogger>
{
    ECS_REGISTRY("game")
    void OnCreated(ecs::Registry&, const ecs::CreatedEntityEvt<Player>& e)
    {
        assert(e.entity.IsAlive());
        std::cout << "[Logger] player spawned with health " << e.entity.GetHealth() << "\n";
    }
    ECS_EVENT(OnCreated, ecs::CreatedEntityEvt<Player>)
};

int main()
{
    ecs::Registry registry = ecs::Registry::Create("game");
    registry.Init();

    // Cook a player, firing both the pre- and post-creation events.
    registry.Commands().Push(ecs::CookCmd<Player, PlayerRecipe>{
        PlayerRecipe{},
        ecs::CookFeedback::PRE_EVT_CALL | ecs::CookFeedback::POST_EVT_CALL
    });

    registry.Update();   // PrefabEvt -> create -> CreatedEntityEvt

    std::cout << "entities: " << registry.Entities().size() << "\n";
}
