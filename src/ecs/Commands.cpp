#include "ecs/registry/Commands.hpp"
#include "ecs/Registry.hpp"


void ecs::CreateEntityCommand::operator()(Registry& registry) const
{
    const auto entity = registry.Entities().Create<Entity>(std::move(*prefab));
    if (onCreated)
        onCreated(entity);
}


void ecs::DeleteEntityCommand::operator()(Registry& registry) const
{
    if (!registry.Entities().IsAlive(entity))
        return;

    if (onDeleted)
        onDeleted(entity);

    registry.Entities().Destroy(entity);
}
