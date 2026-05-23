#pragma once
#include <memory>
#include <vector>

#include "Archetype.hpp"
#include "ecs/components/ComponentRegistrator.hpp"
#include "ecs/utils/ComponentUtils.hpp"


namespace ecs
{

    /**
     * @brief Temporary container for assembling entity component data before creation.
     * PrefabEntity acts as a builder pattern for entities, allowing components to be
     * added one by one before passing to EntitiesManager::Create(). It stores component
     * data in a sparse vector indexed by component ID, making it efficient to transfer
     * data to archetype storage during entity creation.
     *
     * PrefabEntity is intended for short-term use - after entity creation, the prefab
     * is typically cleared and can be reused for building another entity. Component
     * data is stored as byte arrays to support type-erased storage.
     *
     * @note PrefabEntity does NOT own component type information - components are
     *       identified by their static componentId. The component registration system
     *       must be initialized before using PrefabEntity.
     *
     * @example
     * PrefabEntity prefab;
     * prefab.AddComponent<Position>(10.0f, 20.0f);
     * prefab.AddComponent<Velocity>(1.0f, 0.0f);
     * EntityWrapper entity = manager.Create(prefab);
     */
    struct PrefabEntity
    {
        PrefabEntity() = default;
        ~PrefabEntity();

        PrefabEntity(PrefabEntity&&) = default;
        PrefabEntity& operator=(PrefabEntity&&) = default;

        PrefabEntity(const PrefabEntity&) = default;
        PrefabEntity& operator=(const PrefabEntity&) = delete;

        /**
         * @brief Adds a component to the prefab entity.
         * Constructs component in-place using forwarded arguments and stores it
         * at the index corresponding to ComponentRegistrator::GetСomponentId<ComponentCls>().
         *
         * @tparam ComponentCls Component type to add (must be registered)
         * @tparam Args Argument types for component constructor
         * @param args Arguments to forward to component constructor
         *
         * @note If a component of the same ID already exists, it will be replaced.
         * @note The component ID must be less than MAX_COMPONENT_ID (typically 64).
         *
         * @example
         * prefab.AddComponent<Transform>(position, rotation);
         * prefab.AddComponent<Health>(100);
         */
        template<IsComponent ComponentCls, typename... Args>
        void AddComponent(Args&&... args);

        /**
         * @brief Container type for component data.
         * Sparse vector indexed by component ID, each entry holds a unique_ptr
         * to the component's byte array or nullptr if component not present.
         */
        using componentsData_t = std::vector<std::unique_ptr<byte[]>>;

        /**
         * @brief Gets the raw component data storage.
         * Provides access to the internal sparse vector for transfer to
         * archetype storage during entity creation.
         * @return Const reference to component data vector
         */
        [[nodiscard]] const componentsData_t& GetComponentsData() const { return m_dataByComponentsIndex; }

        /**
         * @brief Clears all component data.
         * Destroys all stored components and resets the vector.
         * Called automatically after entity creation to prepare for reuse.
         */
        void clear();

        const Archetype& getArchetype() const;

    private:
        componentsData_t m_dataByComponentsIndex {};        ///< Sparse vector of component data indexed by component ID
        mutable Archetype m_archetype;
        mutable bool m_isDirtyArchetype = false;
    };

    template<IsComponent ComponentCls, typename... Args>
    void PrefabEntity::AddComponent(Args&&... args)
    {
        static const componentId_t componentId = ComponentRegistrator::GetСomponentId<ComponentCls>();
        if(m_dataByComponentsIndex.size() <= componentId)
        {
            m_dataByComponentsIndex.resize(componentId + 1);
        }

        auto ptr = std::make_unique<byte[]>(sizeof(ComponentCls));
        new(ptr.get()) ComponentCls(std::forward<Args>(args)...);

        if (m_dataByComponentsIndex[componentId] == nullptr)
            m_isDirtyArchetype = true;

        m_archetype.set(componentId);
        m_dataByComponentsIndex[componentId] = std::move(ptr);
    }

    inline const Archetype& PrefabEntity::getArchetype() const
    {
        if (m_isDirtyArchetype)
        {
            m_archetype.updateHash();
        }

        return m_archetype;
    }

} // namespace ecs
