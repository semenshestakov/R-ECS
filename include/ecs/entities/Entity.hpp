#pragma once
#include <cstdint>
#include <objc/objc.h>


namespace ecs
{
    class Components;

    using entityId_t = std::uint64_t;


    /**
     * @brief Represents a unique entity in the ECS system with its associated components.
     *
     * The Entity struct combines a unique identifier with a reference to its components,
     * providing a unified interface for entity manipulation. It serves as a handle that
     * allows direct access to both the entity's identity and its component data. The
     * comparison operators enable natural sorting and set operations based on entity IDs,
     * while the dereference operators provide transparent access to the component system.
     */
    struct Entity
    {
        Entity() = delete;
        Entity(const entityId_t a_id, Components& components) : id(a_id), m_components(components) {}
        /// Unique identifier for the entity across the entire system
        entityId_t id;
    protected:
        /// Reference to the entity's associated component container
        std::reference_wrapper<Components> m_components;
    public:

        [[nodiscard]] const Components& components() const { return m_components; }
        [[nodiscard]] Components& components() { return m_components; }

        /**
         * @brief Provides arrow operator access to the entity's components.
         *
         * Enables direct member access to the Components object through the entity,
         * allowing syntax like `entity->getComponent<Transform>()` for intuitive
         * component manipulation. This operator effectively makes the Entity act
         * as a smart pointer to its Components.
         *
         * @return Components& Reference to the entity's components for member access
         */
        [[nodiscard]] Components* operator->() { return &m_components.get(); }
        [[nodiscard]] const Components* operator->() const { return &m_components.get(); }

        /**
         * @brief Provides dereference operator access to the entity's components.
         *
         * Returns a reference to the Components object, allowing the entity to be
         * used directly where a Components reference is expected. This enables
         * syntax like `(*entity).addComponent<Renderable>()` for explicit access
         * to the component container.
         *
         * @return Components& Reference to the entity's components
         */
        [[nodiscard]] Components* operator* () { return &m_components.get(); }
        [[nodiscard]] const Components* operator* () const { return &m_components.get(); }

        /**
         * @brief Three-way comparison operator for ordering entities by their IDs.
         *
         * Enables all six relational operators (==, !=, <, <=, >, >=) for entities
         * through a single implementation. Entities are ordered based on their unique
         * identifiers, which is useful for sorting containers, using entities as keys
         * in ordered maps, or performing set operations on entity collections.
         *
         * @param other The other entity to compare against
         * @return auto The result of comparing the entity IDs (strong ordering)
         */
        auto operator<=>(const Entity& other) const { return id; }
    };
}
