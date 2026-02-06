#pragma once
#include <utility>

#include "Components.hpp"
#include "Entity.hpp"
#include "RegistryFactory.hpp"
#include "utils/RegistryError.hpp"


namespace ecs
{

    /**
     * Base abstract registry with factory ownership.
     * Move-only, non-copyable. All derived registries must provide a factory.
     */
    class AbstractRegistry
    {
    protected:
        /// Protected constructor - requires factory for initialization
        explicit AbstractRegistry(const RegistryFactory& factory) : m_factory(factory) {}

    public:
        AbstractRegistry() = delete;                                            ///< No default construction
        ~AbstractRegistry() = default;                                          ///< Default destructor
        AbstractRegistry(AbstractRegistry&&) noexcept = default;                ///< Move constructible
        AbstractRegistry& operator=(AbstractRegistry&&) noexcept = default;     ///< Move assignable
        AbstractRegistry(const AbstractRegistry&) = delete;                     ///< Non-copyable
        AbstractRegistry& operator=(const AbstractRegistry&) = delete;          ///< Non-copyable

    protected:
        ///< Factory instance for component operations
        RegistryFactory m_factory;
    };


    /**
     * Generic registry implementation templated on collection type.
     *
     * Provides entity-component mapping with creation, lookup, and management.
     * Collection type defines storage strategy (map, dense array, etc.).
     *
     * @tparam Collection Storage type with interface:
     *         - find(entityId_t) -> Components*
     *         - emplace(entityId_t, ComponentsPtr)
     *         - generateId() -> entityId_t (optional)
     */
    template<typename Collection>
    class RegistryT final : public AbstractRegistry
    {
    public:
        /// Construct registry with factory for component creation
        explicit RegistryT(const RegistryFactory& factory) : AbstractRegistry(factory) {}

        /// Find components for entity. Returns nullptr if not found.
        [[nodiscard]] Components* get(const entityId_t entityId) const
        {
            if (auto it = m_data.find(entityId))
                return it;
            return nullptr;
        }

        /// Check if entity exists in registry
        [[nodiscard]] bool contains(const entityId_t entityId) const
        {
            return get(entityId) != nullptr;
        }

        /// Get components for entity. Asserts/throws if entity not found.
        [[nodiscard]] Components& mustGet(const entityId_t entityId) const
        {
            return *get(entityId);
        }

        /**
         * Create components for given entity ID.
         *
         * @param entityId Entity identifier
         * @return Reference to created components
         * @throws error::InvalidEntityId if entity already exists
         */
        Components& create(entityId_t entityId)
        {
            if (contains(entityId))
                throw error::InvalidEntityId("[create] '%llu' id is collected", entityId);

            ComponentsPtr components = m_factory.CreateComponents();
            components->initialize();
            m_data.emplace(entityId, std::move(components));
            return *get(entityId);
        }

        /**
        * Create components with auto-generated entity ID.
        *
        * @return Reference to created components
        * @requires Collection must have generateId() method
        */
        Components& create()
        {
            static_assert(
                requires(Collection proxy) { { proxy.generateId() } -> std::same_as<entityId_t>; },
                "UnorderedMapProxy must have generateId() method returning entityId_t"
                );

            const entityId_t entityId = m_data.generateId();
            return create(entityId);
        }

    private:
        ///< Underlying component storage
        Collection m_data;
    };

}
