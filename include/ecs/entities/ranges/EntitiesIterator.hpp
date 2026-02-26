#pragma once
#include <functional>
#include <optional>
#include "../Entity.hpp"


namespace ecs
{
    struct IEntitiesManager;
}

namespace ecs::ranges
{

    using entityopt_t = std::optional<entityId_t>;
    constexpr entityopt_t entitynull = std::nullopt;

    /**
     * @brief Iterator for traversing collections of entities in a game or ECS system.
     *
     * This iterator provides a flexible way to iterate through entity collections,
     * supporting both standard iteration patterns and custom increment behaviors.
     * It can be compared directly with entity values for convenient range-based
     * operations and integrates with the EntitiesManager for safe entity traversal.
     */
    struct EntitiesIterator
    {
        /**
         * @brief Constructs a default iterator pointing to an invalid/null entity.
         *
         * Initializes the iterator with a null entity value and no custom increment
         * function. This creates an end-of-sequence marker or an uninitialized
         * iterator state that should not be dereferenced.
         */
        EntitiesIterator();

        /**
         * @brief Constructs an iterator pointing to a specific entity.
         *
         * Creates an iterator initialized with the provided entity value. This is
         * typically used to create begin iterators or to point to a specific entity
         * in a collection. The iterator uses default increment behavior.
         *
         * @param value The entity value to which the iterator should point
         */
        explicit EntitiesIterator(entityopt_t value);

        /**
         * @brief Constructs an iterator with custom increment behavior.
         *
         * Creates an iterator that uses the provided function for increment operations.
         * This allows for specialized iteration patterns such as filtered traversal,
         * custom stepping, or iteration over non-contiguous entity collections.
         *
         * @param value The starting entity value for the iterator
         * @param funcInc A move-constructed function that defines increment behavior
         */
        explicit EntitiesIterator(entityopt_t value, std::function<void(EntitiesIterator&)>&& funcInc);

        [[nodiscard]] bool operator==(entityopt_t entityOpt) const;
        [[nodiscard]] bool operator!=(entityopt_t entityOpt) const;

        [[nodiscard]] bool operator==(const EntitiesIterator& other) const;
        [[nodiscard]] bool operator!=(const EntitiesIterator& other) const;

        /**
         * @brief Pre-increment operator that advances the iterator.
         *
         * Moves the iterator to the next entity according to either the default
         * increment behavior or the custom increment function if provided. Returns
         * a reference to the incremented iterator for chaining operations.
         *
         * @return EntitiesIterator& Reference to this iterator after advancement
         */
        EntitiesIterator& operator++();

        /**
         * @brief Post-increment operator that advances the iterator.
         *
         * Advances the iterator to the next entity and returns a copy of the
         * iterator's state before the increment. This supports traditional
         * iterator usage patterns where the pre-increment value is needed.
         *
         * @return EntitiesIterator A copy of the iterator before increment
         */
        EntitiesIterator operator++(int);

        /**
         * @brief Dereference operator that returns the current entity ID.
         *
         * Provides access to the entity ID at the current iterator position.
         * This is typically used to retrieve the entity being pointed to
         * during iteration loops.
         *
         * @return entityId_t The ID of the current entity
         */
        entityId_t operator*() const;

    private:
        entityopt_t m_value = entitynull;                               ///< The current entity value
        const std::function<void(EntitiesIterator&)> m_inc = nullptr;   ///< Custom increment function

        friend IEntitiesManager;
    };

}
