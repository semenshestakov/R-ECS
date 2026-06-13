// ───────────────────────────────────────────────────────────────────────
//  R-ECS example — events
//
//  Shows ECS_EVENT subscriptions and the difference between immediate
//  dispatch (OnEvent) and deferred dispatch (PushEvent, flushed in Update).
//
//  See: docs/en/guides/events.md
// ───────────────────────────────────────────────────────────────────────
#include <iostream>

#include "ecs/ISystem.hpp"
#include "ecs/Registry.hpp"

// ── Event payloads (plain structs) ──────────────────────────────────────
struct DamageEvent { ecs::entityId_t target; float amount; };
struct PlayerDiedEvent { ecs::entityId_t who; };

// ── Two systems that subscribe to the same event ───────────────────────
struct HealthSystem final : ecs::ISystem<HealthSystem>
{
    ECS_REGISTRY("game")

    void OnDamage(ecs::Registry&, const DamageEvent& e)
    {
        std::cout << "[Health] entity " << e.target
                  << " takes " << e.amount << " damage\n";
    }
    ECS_EVENT(OnDamage, DamageEvent)

    void OnPlayerDied(ecs::Registry&, const PlayerDiedEvent& e)
    {
        std::cout << "[Health] entity " << e.who << " died\n";
    }
    ECS_EVENT(OnPlayerDied, PlayerDiedEvent)
};

struct AudioSystem final : ecs::ISystem<AudioSystem>
{
    ECS_REGISTRY("game")

    void OnDamage(ecs::Registry&, const DamageEvent&)
    {
        std::cout << "[Audio]  play hit sound\n";
    }
    ECS_EVENT(OnDamage, DamageEvent)
};

int main()
{
    ecs::Registry registry = ecs::Registry::Create("game");
    registry.Init();

    std::cout << "-- immediate dispatch (OnEvent) --\n";
    // Both HealthSystem and AudioSystem react synchronously, in DAG order.
    registry.Events().OnEvent<DamageEvent>({ /*target*/ 1, /*amount*/ 25.f });

    std::cout << "-- deferred dispatch (PushEvent) --\n";
    registry.Events().PushEvent(PlayerDiedEvent{ /*who*/ 1 });
    std::cout << "(nothing printed yet — waiting for Update)\n";
    registry.Update();   // PlayerDiedEvent handlers run here
}
