#pragma once
#include <concepts>
#include <functional>
#include "Entity.hpp"
#include "ecs/components/Components.hpp"
#include "ranges/EntitiesIterator.hpp"


namespace ecs
{

    /**
     * @brief Abstract interface for entity management systems.
     *
     * Defines the core operations required for managing entities in an ECS architecture.
     * This interface serves as a contract for concrete entity manager implementations,
     * providing iteration capabilities while hiding implementation details. It maintains
     * a protected utility method for iterator manipulation that derived classes can utilize.
     */
    struct IEntitiesManager
    {
        /**
         * @brief Virtual destructor to ensure proper cleanup of derived classes.
         *
         * Enables polymorphic destruction, allowing derived entity managers to
         * clean up their resources correctly when deleted through base class pointers.
         */
        virtual ~IEntitiesManager() = default;

        /**
         * @brief Returns an iterator to the beginning of the entity collection.
         *
         * Provides access to the first entity in the manager's collection. The exact
         * starting point and iteration order are determined by the concrete implementation.
         * This enables range-based for loop support for entity traversal.
         *
         * @return ranges::EntitiesIterator Iterator pointing to the first entity
         */
        [[nodiscard]] virtual ranges::EntitiesIterator begin() const = 0;

        /**
         * @brief Returns an iterator to the end of the entity collection.
         *
         * Provides a sentinel iterator representing the position one past the last
         * entity. This default implementation returns an empty iterator, which
         * derived classes can override if needed for custom end conditions.
         *
         * @return ranges::EntitiesIterator Iterator representing the end marker
         */
        [[nodiscard]] ranges::EntitiesIterator end() const { return {};}

    protected:

        /**
         * @brief Protected utility method to update an iterator's internal value.
         *
         * Allows derived classes to modify iterator state while maintaining encapsulation.
         * This is particularly useful for custom iterator implementations that need
         * to control iterator progression or positioning.
         *
         * @param iterator Reference to the iterator to modify
         * @param value The new entity value to set in the iterator
         */
        static void setValue(ranges::EntitiesIterator& iterator, const ranges::entityopt_t value) { iterator.m_value = value; }
    };

    /**
     * @brief Concept defining the requirements for an entities manager implementation.
     *
     * Specifies the minimal interface that any concrete entity manager must satisfy
     * to be compatible with the EntitiesManager wrapper. This concept ensures that
     * implementations provide entity lookup, ID generation, and component attachment
     * capabilities while inheriting from IEntitiesManager.
     *
     * @tparam T The type to check against the entities manager concept
     */
    template<typename T> concept EntitiesManagerConcept = requires(T manager, entityId_t entityId, ComponentsPtr componentsPtr)
    {
        { manager.find(entityId) } -> std::same_as<Components*>;
        { manager.generateId() } -> std::same_as<entityId_t>;
        { manager.emplace(entityId, std::move(componentsPtr)) } -> std::same_as<void>;
        { std::is_base_of_v<T, IEntitiesManager> };
    };


    /**
     * @brief Polymorphic wrapper for entity manager implementations.
     *
     * Provides a type-erased container for any entity manager that satisfies the
     * EntitiesManagerConcept. This class enables runtime polymorphism without requiring
     * users to work with base class pointers directly. It forwards all entity management
     * operations to the underlying implementation while providing convenient iterator
     * support and factory-style creation.
     */
    class EntitiesManager
    {
    public:
        /**
         * @brief Function object for finding components associated with an entity.
         *
         * Provides lookup functionality that forwards to the underlying manager's
         * find implementation. Returns nullptr if the entity doesn't exist or has
         * no components attached.
         */
        std::function<Components*(entityId_t)> find = nullptr;

    private:
        std::function<void(entityId_t, ComponentsPtr&&)> emplace = nullptr;         ///< Internal emplace function
        std::function<entityId_t()> generateId = nullptr;                           ///< Internal ID generation function
        std::unique_ptr<IEntitiesManager> m_instance = nullptr;                     ///< Type-erased manager instance

    public:
        /**
         * @brief Returns an iterator to the beginning of the managed entity collection.
         *
         * Delegates to the underlying manager instance's begin() method, providing
         * uniform iteration access regardless of the concrete manager type.
         *
         * @return ranges::EntitiesIterator Iterator to the first entity
         */
        [[nodiscard]] ranges::EntitiesIterator begin() const { return m_instance->begin(); }

        /**
         * @brief Returns an iterator to the end of the managed entity collection.
         *
         * Delegates to the underlying manager instance's end() method, providing
         * a sentinel iterator for termination conditions in iteration loops.
         *
         * @return ranges::EntitiesIterator Iterator representing the end marker
         */
        [[nodiscard]] ranges::EntitiesIterator end() const { return m_instance->end(); }

        /**
         * @brief Factory method that creates an EntitiesManager from a concrete implementation.
         *
         * Constructs a type-erased EntitiesManager wrapper around a concrete manager type.
         * This method sets up all necessary function objects to forward calls to the
         * underlying implementation and stores the instance for polymorphic access.
         *
         * @tparam T The concrete manager type that satisfies EntitiesManagerConcept
         * @return EntitiesManager A fully initialized type-erased manager wrapper
         */
        template <EntitiesManagerConcept T>
        static EntitiesManager Create();

        friend class Registry;
    };


    template<EntitiesManagerConcept T>
    EntitiesManager EntitiesManager::Create()
    {
        auto managerUniquePtr = std::make_unique<T>();
        T* managerPtr = managerUniquePtr.get();

        EntitiesManager entitiesManager;

        entitiesManager.find = [managerPtr](entityId_t id) -> Components* {return managerPtr->find(id);};
        entitiesManager.emplace = [managerPtr](entityId_t id, ComponentsPtr&& c) {managerPtr->emplace(id, std::move(c));};
        entitiesManager.generateId = [managerPtr]() -> entityId_t {return managerPtr->generateId();};
        entitiesManager.m_instance = std::move(managerUniquePtr);

        return entitiesManager;
    }

}
