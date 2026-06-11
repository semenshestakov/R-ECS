#include "ecs/Registry.hpp"
#include "ecs/registry/Commands.hpp"


void ecs::CreateEntityCmd::operator()(Registry& registry) const
{
    const auto entity = registry.Entities().Create<Entity>(std::move(*prefab));
    if (onCreated)
        onCreated(entity);
}


void ecs::DeleteEntityCmd::operator()(Registry& registry) const
{
    if (!registry.Entities().IsAlive(entity))
        return;

    if (onDeleted)
        onDeleted(entity);

    registry.Entities().Destroy(entity);
}
