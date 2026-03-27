#ifndef ECS_ENTITY_WRAPPER_HPP
#define ECS_ENTITY_WRAPPER_HPP
#include <functional>
#include "Entity.hpp"
#include "ecs/utils/ComponentUtils.hpp"


namespace ecs
{
    class EntitiesManager;


    struct EntityWrapper
    {
        EntityWrapper() = delete;
        EntityWrapper(const Entity& entity, EntitiesManager& entitiesManager);

        [[nodiscard]] bool IsAlive() const;
        void SelfDestroy() const;

        template<DerivedComponent ComponentCls>
        [[nodiscard]] ComponentCls& GetComponent();

        template<DerivedComponent ComponentCls>
        [[nodiscard]] const ComponentCls& GetComponent() const;

        template<DerivedComponent ComponentCls>
        [[nodiscard]] ComponentCls* TryGetComponent();

        template<DerivedComponent ComponentCls>
        [[nodiscard]] const ComponentCls* TryGetComponent() const;

        [[nodiscard]] byte* GetComponentData(componentId_t componentId);
        [[nodiscard]] const byte* GetComponentData(componentId_t componentId) const;

        [[nodiscard]] const Entity& getEntity() const { return m_entity; }

    private:
        Entity m_entity;
        std::reference_wrapper<EntitiesManager> m_managerRef;
    };

} // namespace ecs
#endif
#include "detail/EntityWrapper.ipp"
