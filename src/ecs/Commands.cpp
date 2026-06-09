#include "ecs/registry/Commands.hpp"
#include "ecs/Registry.hpp"


void ecs::CreateEntityCommand::operator()() const
{
    const auto entity = registry.get().Entities().Create<Entity>(std::move(*prefab));
    if (onCreated)
        onCreated(entity);
}


void ecs::DeleteEntityCommand::operator()() const
{
    if (!registry.get().Entities().IsAlive(entity))
        return;

    if (onDeleted)
        onDeleted(entity);

    registry.get().Entities().Destroy(entity);
}
