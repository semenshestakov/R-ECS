#ifndef ENTITIES_MANAGER_HPP
#define ENTITIES_MANAGER_HPP

#include <cassert>
#include <queue>

#include "EntitiesArchetypeStorage.hpp"
#include "Entity.hpp"
#include "EntityWrapper.hpp"


namespace ecs
{

    class EntitiesManager final
    {
    public:
        EntitiesManager();
        ~EntitiesManager();

        EntitiesManager(const EntitiesManager& other) = delete;
        EntitiesManager& operator=(const EntitiesManager& other) = delete;

        EntitiesManager(EntitiesManager&&) noexcept = default;
        EntitiesManager& operator=(EntitiesManager&&) noexcept = default;

        EntityWrapper Create(PrefabEntity& prefabEntity);
        EntityWrapper Create(PrefabEntity&& prefabEntity);
        void Destroy(const Entity& entity);
        void Destroy(const EntityWrapper& entity);
        [[nodiscard]] bool IsAlive(const Entity& entity) const;
        [[nodiscard]] bool IsAlive(const EntityWrapper& entity) const;

        template<DerivedComponent ComponentCls> [[nodiscard]] ComponentCls& GetComponent(const Entity& entity);
        template<DerivedComponent ComponentCls> [[nodiscard]] const ComponentCls& GetComponent(const Entity& entity) const;

        template<DerivedComponent ComponentCls> [[nodiscard]] ComponentCls* TryGetComponent(const Entity& entity);
        template<DerivedComponent ComponentCls> [[nodiscard]] const ComponentCls* TryGetComponent(const Entity& entity) const;

        [[nodiscard]] byte* GetComponentData(const Entity& entity, componentId_t componentId);
        [[nodiscard]] const byte* GetComponentData(const Entity& entity, componentId_t componentId) const;

        [[nodiscard]] std::size_t size() const { return m_isAliveEntitiesCount; }

        template<DerivedComponent... ComponentCls>
        auto view();

    private:
        std::size_t m_isAliveEntitiesCount = 0;
        entityId_t m_lastEntityId = INVALID_ENTITY_ID + 1;
        std::queue<Entity> m_freeEntities;

        EntitiesArchetypeStorage m_storage;
        std::vector<entityVersion_t> m_versionByEntityIndex;
        std::vector<ArchetypedChunkEntityLocation> m_entitiesLocationByEntityIndex;

        void resize(std::size_t size);
    };

} // namespace ecs
#endif
#include "detail/EntitiesManager.ipp"
