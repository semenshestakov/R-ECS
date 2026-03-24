#include "ecs/Registry.hpp"
#include "ecs/utils/RegistryError.hpp"


namespace ecs
{

    Registry::Registry(EntitiesManager&& entitiesManager, const ComponentsManager& componentsManager) :
        m_entitiesManager(std::move(entitiesManager)),
        m_componentsManager(componentsManager)
    {
    }

    Registry::Registry(
        EntitiesManager&& entitiesManager, const ComponentsManager& componentsManager, const SystemsManager& systemManager
        )  :
        m_entitiesManager(std::move(entitiesManager)),
        m_componentsManager(componentsManager),
        m_systemManager(systemManager)
    {
    }

    Components* Registry::get(const entityId_t entityId) const
    {
        if (auto it = m_entitiesManager.find(entityId))
            return it;
        return nullptr;
    }

    collections::Context& Registry::ctx()
    {
        return m_context;
    }

    bool Registry::contains(const entityId_t entityId) const
    {
        return get(entityId) != nullptr;
    }

    std::size_t Registry::size() const
    {
        return m_entitiesManager.size();
    }

    Components & Registry::mustGet(const entityId_t entityId) const
    {
        return *get(entityId);
    }

    bool Registry::Init(void* args /* = nullptr */)
    {
        return m_systemManager.Init({"", args, m_eventSystem});
    }

    Components& Registry::Create(const entityId_t entityId)
    {
        if (contains(entityId))
            throw error::InvalidEntityId("[create] '%llu' id is collected", entityId);

        ComponentsPtr components = m_componentsManager.CreateComponents();
        components->initialize();
        m_entitiesManager.emplace(entityId, std::move(components));
        return *get(entityId);
    }

    Entity Registry::Create()
    {
        const entityId_t entityId = m_entitiesManager.generateId();
        return {entityId, Create(entityId)};
    }

    void Registry::Update(const std::optional<updateTag_t> updateTag /* = nullopt */)
    {
        m_systemManager.Update(*this, updateTag);
    }

} // namespace ecs
