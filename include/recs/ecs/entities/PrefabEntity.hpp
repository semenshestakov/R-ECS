#ifndef PREFAB_ENTITY_HPP
#define PREFAB_ENTITY_HPP
#include <memory>
#include <vector>

#include "../components/Utils.hpp"
#include "Archetype.hpp"
#include "ecs/components/ComponentRegistrator.hpp"


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
         * at the index corresponding to ComponentRegistrator::GetComponentId<ComponentCls>().
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
        ComponentCls& AddComponent(Args&&... args);
        
        template<IsComponent ComponentCls>
        [[nodiscard]] ComponentCls* TryGetComponent();

        template<IsComponent ComponentCls>
        [[nodiscard]] const ComponentCls* TryGetComponent() const;

        template<IsComponent ComponentCls>
        [[nodiscard]] ComponentCls& GetComponent();

        template<IsComponent ComponentCls>
        [[nodiscard]] const ComponentCls& GetComponent() const;

        /**
         * @brief Custom deleter for component slots acquired from ComponentFreeList.
         *
         * Used as the deleter type of unique_ptr<byte[], PoolDeleter> inside
         * componentsData_t. On destruction, returns the raw slot back to the
         * per-type free list via ComponentRegistrator::GetInfo(componentId).poolRelease,
         * avoiding a call to the system deallocator.
         *
         * @note Does NOT call the component destructor — that responsibility belongs
         *       to the PrefabEntity destructor and clear(), which must run it explicitly
         *       before the unique_ptr goes out of scope.
         */
        struct PoolDeleter
        {
            componentId_t componentId { INVALID_COMPONENT_ID }; ///< ID of the component type whose free list owns this slot.
            void operator()(byte* ptr) const;
        };

        /**
         * @brief Container type for component data.
         * Sparse vector indexed by component ID, each entry holds a unique_ptr
         * to the component's byte array (acquired from ComponentFreeList) or nullptr if component not present.
         */
        using componentsData_t = std::vector<std::unique_ptr<byte[], PoolDeleter>>;

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

        /**
         * @brief Returns the archetype derived from currently added components.
         *
         * Recomputes the archetype hash lazily when the component set has changed
         * since the last call (i.e. when m_isDirtyArchetype is true).
         *
         * @return Const reference to the up-to-date archetype.
         */
        const Archetype& getArchetype() const;

    private:
        componentsData_t m_dataByComponentsIndex {};        ///< Sparse vector of component data indexed by component ID
        mutable Archetype m_archetype;              ///< Cached archetype built from all added component IDs.
        mutable bool m_isDirtyArchetype = false;    ///< True when the component set changed and the archetype hash needs recomputation.
    };

    /**
     * @brief Constrains a forwarding reference to PrefabEntity.
     * Allows only `const PrefabEntity&` (copy path) or `PrefabEntity&&` (move path).
     * Mutable lvalue references are rejected to enforce const-correctness.
     */
    template<typename T>
    concept PrefabEntityRef = std::same_as<std::remove_cvref_t<T>, PrefabEntity>;

} // namespace ecs
#endif
#include "detail/PrefabEntity.ipp"
