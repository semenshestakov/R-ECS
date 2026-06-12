#pragma once
#include "../EntitiesManager.hpp"


inline ecs::EntitiesManager::EntitiesManager() { resize(256); }

inline ecs::EntitiesManager::~EntitiesManager()
{
    for(entityId_t entityId = INVALID_ENTITY_ID + 1; entityId < m_lastEntityId; ++entityId)
    {
        if(m_versionByEntityIndex[entityId] != INVALID_ENTITY_VERSION)
            m_storage.Destroy(m_entitiesLocationByEntityIndex[entityId]);
    }
}

template<ecs::EntityConcept ReturnType, ecs::PrefabEntityRef PrefabRef>
ReturnType ecs::EntitiesManager::Create(PrefabRef&& prefabEntity)
{
    static_assert(std::is_same_v<ReturnType, ecs::Entity> || ecs::EntityWrapperLike<ReturnType>,
        "ReturnType must be Entity or an EntityWrapper subclass without data members. "
        "Use components for state, not wrapper fields.");

    Entity entity{};

    if(!m_freeEntities.empty())
    {
        entity = m_freeEntities.front();
        if(entity.version == MAX_ENTITY_VERSION)
            entity.version = INVALID_ENTITY_VERSION + 1; ///< reuse first entity.version
        else
            ++entity.version; ///< inc free entity

        m_freeEntities.pop();
    } else
    {
        entity.id = m_lastEntityId++;
        assert(entity.id != MAX_ENTITY_ID and entity.id != INVALID_ENTITY_ID);

        entity.version = INVALID_ENTITY_VERSION + 1;
        if(entity.id >= m_versionByEntityIndex.size())
        {
            resize(std::max<std::size_t>(m_versionByEntityIndex.size(), 1) * 2);
        }
    }

    assert(entity.id < m_lastEntityId);
    m_entitiesLocationByEntityIndex[entity.id] = m_storage.Create(std::forward<PrefabRef>(prefabEntity), entity.id);
    m_versionByEntityIndex[entity.id] = entity.version;
    ++m_isAliveEntitiesCount;

    if constexpr (EntityWrapperLike<ReturnType>)
        return ReturnType{entity, *this};
    else
        return entity;
}

template<typename... Args>
void ecs::EntitiesManager::AddComponents(const Entity& entity, Args&&... args)
{
    static_assert(sizeof...(Args) > 0, "AddComponents requires at least one component");

    if (!IsAlive(entity))
        return;

    const ArchetypedChunkEntityLocation oldLocation = m_entitiesLocationByEntityIndex[entity.id];

    Archetype argsArchetype;
    (argsArchetype.set(ComponentRegistrator::GetСomponentId<std::remove_cvref_t<Args>>()), ...);

    const Archetype oldArchetype = m_storage.getArchetype(oldLocation.archetypeIndex);

    if (argsArchetype.isSubsetOf(oldArchetype))
    {
        ([&]
        {
            using Component = std::remove_cvref_t<Args>;
            byte* dest = m_storage.GetComponentData(oldLocation, ComponentRegistrator::GetСomponentId<Component>());
            *std::bit_cast<Component*>(dest) = std::forward<Args>(args);
        }(), ...);
        return;
    }

    Archetype newArchetype = oldArchetype;
    (newArchetype.set(ComponentRegistrator::GetСomponentId<std::remove_cvref_t<Args>>()), ...);
    newArchetype.updateHash();

    const auto [newLocation, swapRemovedEntityId] =
        m_storage.MigrateEntity(oldLocation, newArchetype, entity.id);

    if (swapRemovedEntityId != INVALID_ENTITY_ID)
        m_entitiesLocationByEntityIndex[swapRemovedEntityId].chunkEntityIndex = oldLocation.chunkEntityIndex;

    ([&]
    {
        using Comp = std::remove_cvref_t<Args>;
        const componentId_t componentId = ComponentRegistrator::GetСomponentId<Comp>();
        byte* dest = m_storage.GetComponentData(newLocation, componentId);

        if (oldArchetype.test(componentId))
            *std::bit_cast<Comp*>(dest) = std::forward<Args>(args);
        else
            new(dest) Comp(std::forward<Args>(args));
    }(), ...);

    m_entitiesLocationByEntityIndex[entity.id] = newLocation;
}

template<ecs::IsComponent... Args>
void ecs::EntitiesManager::RemoveComponents(const Entity& entity)
{
    static_assert(sizeof...(Args) > 0, "RemoveComponents requires at least one component");

    if (!IsAlive(entity))
        return;

    const ArchetypedChunkEntityLocation oldLocation = m_entitiesLocationByEntityIndex[entity.id];
    const Archetype oldArchetype = m_storage.getArchetype(oldLocation.archetypeIndex);

    Archetype newArchetype = oldArchetype;
    (newArchetype.reset(ComponentRegistrator::GetСomponentId<Args>()), ...);

    if (newArchetype == oldArchetype) // none of the requested components were present
        return;

    if (newArchetype.min() == collection::BitSet::INVALID_INDEX) // every component removed -> entity no longer exists
    {
        Destroy(entity);
        return;
    }

    newArchetype.updateHash();

    const auto [newLocation, swapRemovedEntityId] =
        m_storage.MigrateEntity(oldLocation, newArchetype, entity.id);

    if (swapRemovedEntityId != INVALID_ENTITY_ID)
        m_entitiesLocationByEntityIndex[swapRemovedEntityId].chunkEntityIndex = oldLocation.chunkEntityIndex;

    m_entitiesLocationByEntityIndex[entity.id] = newLocation;
}

template<ecs::EntityConcept InputEntityType>
void ecs::EntitiesManager::Destroy(const InputEntityType& entity)
{
    if(!IsAlive(entity))
        return;

    assert(entity.getId() < m_lastEntityId);
    assert(m_versionByEntityIndex.size() == m_entitiesLocationByEntityIndex.size());

    if (
        const entityId_t migratedEntityId = m_storage.Destroy(m_entitiesLocationByEntityIndex[entity.getId()]);
        migratedEntityId != INVALID_ENTITY_ID
        )
    {
        m_versionByEntityIndex[migratedEntityId] = entity.getVersion();
        m_entitiesLocationByEntityIndex[migratedEntityId].chunkEntityIndex = m_entitiesLocationByEntityIndex[entity.getId()].chunkEntityIndex;
    }

    m_versionByEntityIndex[entity.getId()] = {};
    m_freeEntities.emplace(entity);
    --m_isAliveEntitiesCount;
}


template<ecs::EntityConcept InputEntityType>
bool ecs::EntitiesManager::IsAlive(const InputEntityType& entity) const
{
    if (entity.getId() == INVALID_ENTITY_ID || entity.getVersion() == INVALID_ENTITY_VERSION)
        return false;

    if(entity.getId() >= m_versionByEntityIndex.size())
        return false;

    if(m_versionByEntityIndex[entity.getId()] != entity.getVersion())
        return false;

    return true;
}

inline ecs::byte* ecs::EntitiesManager::GetComponentData(const Entity& entity, const componentId_t componentId)
{
    if(!IsAlive(entity))
        return nullptr;

    return m_storage.GetComponentData(m_entitiesLocationByEntityIndex[entity.id], componentId);
}

inline const ecs::byte* ecs::EntitiesManager::GetComponentData(const Entity& entity, const componentId_t componentId) const
{
    if(!IsAlive(entity))
        return nullptr;

    return m_storage.GetComponentData(m_entitiesLocationByEntityIndex[entity.id], componentId);
}

template<ecs::IsComponent ComponentCls>
ComponentCls* ecs::EntitiesManager::TryGetComponent(const Entity& entity)
{
    return std::bit_cast<ComponentCls*>(GetComponentData(entity, ComponentRegistrator::GetСomponentId<ComponentCls>()));
}

template<ecs::IsComponent ComponentCls>
const ComponentCls* ecs::EntitiesManager::TryGetComponent(const Entity& entity) const
{
    return std::bit_cast<const ComponentCls*>(GetComponentData(entity, ComponentRegistrator::GetСomponentId<ComponentCls>()));
}

template<ecs::IsComponent ComponentCls>
ComponentCls& ecs::EntitiesManager::GetComponent(const Entity& entity)
{
    ComponentCls* component = TryGetComponent<ComponentCls>(entity);
    assert(component != nullptr);
    return *component;
}

template<ecs::IsComponent ComponentCls>
const ComponentCls& ecs::EntitiesManager::GetComponent(const Entity& entity) const
{
    const ComponentCls* component = TryGetComponent<ComponentCls>(entity);
    assert(component != nullptr);
    return *component;
}

template<ecs::IsComponent... ComponentCls>
auto ecs::EntitiesManager::view()
{
    struct View
    {
        View() = delete;
        explicit View(EntitiesArchetypeStorage* storage) : m_storage(storage) {}

        auto begin() const { return m_storage->begin<ComponentCls...>(); }
        auto end() const { return m_storage->end<ComponentCls...>(); }

    private:
        EntitiesArchetypeStorage* m_storage;
    };

    return View(&m_storage);
}

inline void ecs::EntitiesManager::resize(const std::size_t size)
{
    m_versionByEntityIndex.resize(size);
    m_entitiesLocationByEntityIndex.resize(size);
}
