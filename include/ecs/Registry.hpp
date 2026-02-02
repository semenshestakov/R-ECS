#pragma once
#include "Components.hpp"
#include "Entity.hpp"
#include "RegistryFactory.hpp"
#include "utils/RegistryError.hpp"


namespace ecs
{

    class AbstractRegistry
    {

    protected:
        AbstractRegistry(const RegistryFactory& factory) : m_factory(factory) {}

    public:
        AbstractRegistry() = delete;
        ~AbstractRegistry() = default;
        AbstractRegistry(AbstractRegistry&&) noexcept = default;
        AbstractRegistry& operator=(AbstractRegistry&&) noexcept = default;

        // Delete Copy
        AbstractRegistry(const AbstractRegistry&) = delete;
        AbstractRegistry& operator=(const AbstractRegistry&) = delete;

    protected:
        RegistryFactory m_factory;

    };

    template<typename Collection>
    class RegistryT : public AbstractRegistry
    {
    public:
        RegistryT(const RegistryFactory& factory) : AbstractRegistry(factory) {}

        [[nodiscard]] Components* get(const entityId_t entityId) const
        {
            if (auto it = m_data.find(entityId))
                return it;
            return nullptr;
        }

        [[nodiscard]] bool contains(const entityId_t entityId) const
        {
            return get(entityId) != nullptr;
        }

        [[nodiscard]] Components& mustGet(const entityId_t entityId) const
        {
            return *get(entityId);
        }

        Components& create(entityId_t entityId)
        {
            if (contains(entityId))
                throw error::InvalidEntityId("[create] '%llu' id is collected", entityId);

            ComponentsPtr components = m_factory.createComponents();
            components->initialize();
            m_data.emplace(entityId, std::move(components));
            return *get(entityId);
        }

        Components& create()
        {
            static_assert(
                requires(Collection proxy) { { proxy.generateId() } -> std::same_as<entityId_t>; },
                "_UnorderedMapProxy must have generateId() method returning entityId_t"
                );

            const entityId_t entityId = m_data.generateId();
            return create(entityId);
        }

    private:
        Collection m_data;

    };

}
