#pragma once
#include <tuple>
#include <utility>
#include "../EntitiesManager.hpp"


namespace ecs::detail
{
    /**
     * @brief Range adapter forwarding begin()/end() to storage iteration.
     * @tparam StorageArgs Argument pack passed straight to EntitiesArchetypeStorage.
     */
    template<typename... StorageArgs>
    struct StorageView
    {
        EntitiesArchetypeStorage* m_storage;

        [[nodiscard]] auto begin() const { return m_storage->template begin<StorageArgs...>(); }
        [[nodiscard]] auto end()   const { return m_storage->template end<StorageArgs...>(); }
    };

    /**
     * @brief Range adapter that replaces the leading Entity of each yielded tuple with
     *        a constructed wrapper handle.
     *
     * The underlying storage range is Entity-led (its value type is
     * `std::tuple<Entity, Components...>`); this adapter maps element 0 to
     * `Wrapper{entity, manager}` and forwards the remaining component references.
     *
     * @tparam Wrapper     EntityWrapperLike handle type to yield.
     * @tparam StorageArgs Storage argument pack (always Entity-led).
     */
    template<typename Wrapper, typename... StorageArgs>
    struct WrapperView
    {
        EntitiesArchetypeStorage* m_storage;
        EntitiesManager* m_manager;

        using inner_iterator_t =
            decltype(std::declval<EntitiesArchetypeStorage&>().template begin<StorageArgs...>());

        struct iterator
        {
            inner_iterator_t m_it;
            EntitiesManager* m_manager;

            [[nodiscard]] auto operator*() const
            {
                return std::apply(
                    [mgr = m_manager](auto&& entity, auto&&... rest)
                    {
                        return std::tuple_cat(
                            std::tuple<Wrapper>(Wrapper{entity, *mgr}),
                            std::forward_as_tuple(std::forward<decltype(rest)>(rest)...)
                        );
                    },
                    *m_it);
            }

            iterator& operator++() { ++m_it; return *this; }
            iterator operator++(int) { iterator copy = *this; ++m_it; return copy; }

            [[nodiscard]] bool operator==(const iterator& other) const { return m_it == other.m_it; }
            [[nodiscard]] bool operator!=(const iterator& other) const { return m_it != other.m_it; }
        };

        [[nodiscard]] auto begin() const { return iterator{m_storage->template begin<StorageArgs...>(), m_manager}; }
        [[nodiscard]] auto end()   const { return iterator{m_storage->template end<StorageArgs...>(), m_manager}; }
    };

} // namespace ecs::detail


inline ecs::EntitiesManager::EntitiesManager() { resize(256); }

inline ecs::EntitiesManager::~EntitiesManager() = default;

template<ecs::EntityConcept ReturnType, ecs::PrefabEntityRef PrefabRef>
ReturnType ecs::EntitiesManager::Create(PrefabRef&& prefabEntity)
{
    static_assert(std::is_same_v<ReturnType, ecs::Entity> || ecs::EntityWrapperLike<ReturnType>,
        "ReturnType must be Entity or an EntityWrapper subclass without data members. "
        "Use components for state, not wrapper fields.");

    ECS_ASSERT_MAIN_THREAD("EntitiesManager::Create");

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
    m_entitiesLocationByEntityIndex[entity.id] = m_storage.Create(std::forward<PrefabRef>(prefabEntity), entity);
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

    ECS_ASSERT_MAIN_THREAD("EntitiesManager::AddComponents");

    if (!IsAlive(entity))
        return;

    const ArchetypedChunkEntityLocation oldLocation = m_entitiesLocationByEntityIndex[entity.id];

    Archetype argsArchetype;
    (argsArchetype.set(ComponentRegistrator::GetComponentId<std::remove_cvref_t<Args>>()), ...);

    const Archetype oldArchetype = m_storage.getArchetype(oldLocation.archetypeIndex);

    if (argsArchetype.isSubsetOf(oldArchetype))
    {
        ([&]
        {
            using Component = std::remove_cvref_t<Args>;
            if constexpr (!IsTag<Component>)
            {
                byte* dest = m_storage.GetComponentData(oldLocation, ComponentRegistrator::GetComponentId<Component>());
                *std::bit_cast<Component*>(dest) = std::forward<Args>(args);
            }
        }(), ...);
        return;
    }

    Archetype newArchetype = oldArchetype;
    (newArchetype.set(ComponentRegistrator::GetComponentId<std::remove_cvref_t<Args>>()), ...);
    newArchetype.updateHash();

    const auto [newLocation, swapRemovedEntity] =
        m_storage.MigrateEntity(oldLocation, newArchetype, entity);

    if (swapRemovedEntity.id != INVALID_ENTITY_ID)
        m_entitiesLocationByEntityIndex[swapRemovedEntity.id].chunkEntityIndex = oldLocation.chunkEntityIndex;

    ([&]
    {
        using Comp = std::remove_cvref_t<Args>;
        if constexpr (!IsTag<Comp>)
        {
            const componentId_t componentId = ComponentRegistrator::GetComponentId<Comp>();
            byte* dest = m_storage.GetComponentData(newLocation, componentId);

            if (oldArchetype.test(componentId))
                *std::bit_cast<Comp*>(dest) = std::forward<Args>(args);
            else
                new(dest) Comp(std::forward<Args>(args));
        }
    }(), ...);

    m_entitiesLocationByEntityIndex[entity.id] = newLocation;
}

template<ecs::IsTag... Tags>
void ecs::EntitiesManager::AddTag(const Entity& entity)
{
    static_assert(sizeof...(Tags) > 0, "AddTag requires at least one tag");

    ECS_ASSERT_MAIN_THREAD("EntitiesManager::AddTag");

    if (!IsAlive(entity))
        return;

    const ArchetypedChunkEntityLocation oldLocation = m_entitiesLocationByEntityIndex[entity.id];
    const Archetype oldArchetype = m_storage.getArchetype(oldLocation.archetypeIndex);

    Archetype newArchetype = oldArchetype;
    (newArchetype.set(ComponentRegistrator::GetComponentId<Tags>()), ...);

    if (newArchetype == oldArchetype) // all requested tags already present
        return;

    newArchetype.updateHash();

    const auto [newLocation, swapRemovedEntity] =
        m_storage.MigrateEntity(oldLocation, newArchetype, entity);

    if (swapRemovedEntity.id != INVALID_ENTITY_ID)
        m_entitiesLocationByEntityIndex[swapRemovedEntity.id].chunkEntityIndex = oldLocation.chunkEntityIndex;

    m_entitiesLocationByEntityIndex[entity.id] = newLocation;
}

template<ecs::IsTag... Tags>
void ecs::EntitiesManager::RemoveTag(const Entity& entity)
{
    static_assert(sizeof...(Tags) > 0, "RemoveTag requires at least one tag");
    RemoveComponents<Tags...>(entity);
}

template<ecs::IsComponent... Args>
void ecs::EntitiesManager::RemoveComponents(const Entity& entity)
{
    static_assert(sizeof...(Args) > 0, "RemoveComponents requires at least one component");

    ECS_ASSERT_MAIN_THREAD("EntitiesManager::RemoveComponents");

    if (!IsAlive(entity))
        return;

    const ArchetypedChunkEntityLocation oldLocation = m_entitiesLocationByEntityIndex[entity.id];
    const Archetype oldArchetype = m_storage.getArchetype(oldLocation.archetypeIndex);

    Archetype newArchetype = oldArchetype;
    (newArchetype.reset(ComponentRegistrator::GetComponentId<Args>()), ...);

    if (newArchetype == oldArchetype) // none of the requested components were present
        return;

    if (newArchetype.min() == collections::BitSet::INVALID_INDEX) // every component removed -> entity no longer exists
    {
        Destroy(entity);
        return;
    }

    newArchetype.updateHash();

    const auto [newLocation, swapRemovedEntity] =
        m_storage.MigrateEntity(oldLocation, newArchetype, entity);

    if (swapRemovedEntity.id != INVALID_ENTITY_ID)
        m_entitiesLocationByEntityIndex[swapRemovedEntity.id].chunkEntityIndex = oldLocation.chunkEntityIndex;

    m_entitiesLocationByEntityIndex[entity.id] = newLocation;
}

template<ecs::EntityConcept InputEntityType>
void ecs::EntitiesManager::Destroy(const InputEntityType& entity)
{
    ECS_ASSERT_MAIN_THREAD("EntitiesManager::Destroy");

    if(!IsAlive(entity))
        return;

    assert(entity.getId() < m_lastEntityId);
    assert(m_versionByEntityIndex.size() == m_entitiesLocationByEntityIndex.size());

    if (
        const Entity migratedEntity = m_storage.Destroy(m_entitiesLocationByEntityIndex[entity.getId()]);
        migratedEntity.id != INVALID_ENTITY_ID
        )
    {
        m_versionByEntityIndex[migratedEntity.id] = entity.getVersion();
        m_entitiesLocationByEntityIndex[migratedEntity.id].chunkEntityIndex = m_entitiesLocationByEntityIndex[entity.getId()].chunkEntityIndex;
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

    return m_storage.GetComponentData(m_entitiesLocationByEntityIndex.at(entity.id), componentId);
}

inline const ecs::byte* ecs::EntitiesManager::GetComponentData(const Entity& entity, const componentId_t componentId) const
{
    if(!IsAlive(entity))
        return nullptr;

    return m_storage.GetComponentData(m_entitiesLocationByEntityIndex[entity.id], componentId);
}

inline const ecs::Archetype& ecs::EntitiesManager::GetArchetype(const Entity& entity) const
{
    assert(IsAlive(entity));
    return m_storage.getArchetype(m_entitiesLocationByEntityIndex[entity.id].archetypeIndex);
}

template<ecs::IsComponent ComponentCls>
ComponentCls* ecs::EntitiesManager::TryGetComponent(const Entity& entity)
{
    return std::bit_cast<ComponentCls*>(GetComponentData(entity, ComponentRegistrator::GetComponentId<ComponentCls>()));
}

template<ecs::IsComponent ComponentCls>
const ComponentCls* ecs::EntitiesManager::TryGetComponent(const Entity& entity) const
{
    return std::bit_cast<const ComponentCls*>(GetComponentData(entity, ComponentRegistrator::GetComponentId<ComponentCls>()));
}

template<ecs::IsComponent ComponentCls>
ComponentCls& ecs::EntitiesManager::GetComponent(const Entity& entity)
{
    assert(IsAlive(entity));
    return m_storage.template GetComponent<ComponentCls>(m_entitiesLocationByEntityIndex[entity.id]);
}

template<ecs::IsComponent ComponentCls>
const ComponentCls& ecs::EntitiesManager::GetComponent(const Entity& entity) const
{
    assert(IsAlive(entity));
    return m_storage.template GetComponent<ComponentCls>(m_entitiesLocationByEntityIndex[entity.id]);
}

template<typename... Args>
auto ecs::EntitiesManager::GetComponents(const Entity& entity)
{
    assert(IsAlive(entity));
    return std::tuple<view_element_t<Args>...>(
        [&]() -> view_element_t<Args>
        {
            if constexpr (is_optional_component_v<Args>)
                return TryGetComponent<component_bare_t<Args>>(entity);
            else
                return GetComponent<component_bare_t<Args>>(entity);
        }()...
    );
}

template<typename... Args>
auto ecs::EntitiesManager::GetComponents(const Entity& entity) const
{
    assert(IsAlive(entity));
    return std::tuple<view_const_element_t<Args>...>(
        [&]() -> view_const_element_t<Args>
        {
            if constexpr (is_optional_component_v<Args>)
                return TryGetComponent<component_bare_t<Args>>(entity);
            else
                return GetComponent<component_bare_t<Args>>(entity);
        }()...
    );
}

template<typename... Args>
auto ecs::EntitiesManager::view()
{
    if constexpr (sizeof...(Args) == 0)
        return detail::StorageView<>{&m_storage};
    else
        return viewDispatch<Args...>();
}

template<typename Head, typename... Tail>
auto ecs::EntitiesManager::viewDispatch()
{
    static_assert(
        ((!std::same_as<component_bare_t<Tail>, Entity> && !EntityWrapperLike<component_bare_t<Tail>>) && ...),
        "view<Head, Tail...>: only the Head argument selects the yielded handle. "
        "Every Tail argument must be a component (Component / Component*) or a filter-only "
        "tag — a second Entity or EntityWrapper (including a wrapper-tag) is not allowed.");

    if constexpr (std::same_as<Head, Entity>)
    {
        return detail::StorageView<Head, Tail...>{&m_storage};
    }
    else if constexpr (EntityWrapperLike<Head>)
    {
        // Wrapper handle. A wrapper-tag keeps its bit in the filter (Entity-led storage
        // pack keeps Head); a pure wrapper only selects the handle and is dropped.
        if constexpr (IsTag<Head>)
            return detail::WrapperView<Head, Entity, Head, Tail...>{&m_storage, this};
        else
            return detail::WrapperView<Head, Entity, Tail...>{&m_storage, this};
    }
    else if constexpr (IsTag<Head>)
    {
        // Plain tag head: yield the Entity, keep the tag as a filter bit.
        return detail::StorageView<Entity, Head, Tail...>{&m_storage};
    }
    else
    {
        // Normal component head: unchanged component-tuple behaviour.
        return detail::StorageView<Head, Tail...>{&m_storage};
    }
}

template<typename... Args>
auto ecs::EntitiesManager::chunkView()
{
    struct ChunkRange
    {
        ChunkRange() = delete;
        explicit ChunkRange(EntitiesArchetypeStorage* storage) : m_storage(storage) {}

        auto begin() const { return m_storage->chunksBegin<Args...>(); }
        auto end() const { return m_storage->chunksEnd<Args...>(); }

    private:
        EntitiesArchetypeStorage* m_storage;
    };

    return ChunkRange(&m_storage);
}

inline void ecs::EntitiesManager::resize(const std::size_t size)
{
    m_versionByEntityIndex.resize(size);
    m_entitiesLocationByEntityIndex.resize(size);
}
