#pragma once
#include "EntitiesIterator.hpp"



namespace ecs::ranges::view
{

    /**
     * @brief CRTP base class providing iterator functionality for entity view types.
     *
     * This template implements the common iterator interface and behavior for all
     * entity view types in the system. It uses the Curiously Recurring Template Pattern
     * (CRTP) to enable polymorphic behavior without virtual functions. The class
     * manages an underlying EntitiesIterator and provides standard iterator operations
     * while allowing derived classes to customize dereferencing and increment behavior.
     *
     * @tparam Derived The derived view class that inherits from this template
     */
    template<typename Derived>
    struct DerivedViews
    {
        DerivedViews() = delete;

        /**
         * @brief Constructs a view with an iterator and reference to the entities manager.
         *
         * Initializes the view with a specific iterator position and a reference to the
         * manager that owns the entities. This allows the view to access entity data
         * and components during iteration.
         *
         * @param iterator The iterator pointing to the current entity position
         * @param manager Reference to the entities manager for component access
         */
        DerivedViews(EntitiesIterator&& iterator, EntitiesManager& manager) : m_iterator(std::move(iterator)), m_manager(manager) {}

        [[nodiscard]] bool operator==(const Derived& other) const { return m_iterator == other.m_iterator;}
        [[nodiscard]] bool operator!=(const Derived& other) const { return m_iterator != other.m_iterator;}

        [[nodiscard]] bool operator==(const EntitiesIterator& it) const { return m_iterator == it;}
        [[nodiscard]] bool operator!=(const EntitiesIterator& it) const { return m_iterator != it;}

        /**
         * @brief Advances the iterator by one position (basic increment).
         *
         * Moves the underlying iterator to the next entity in the collection.
         * This is the base increment operation that derived classes can extend.
         */
        void increment() { ++m_iterator; }

        /**
         * @brief Pre-increment operator that advances the view.
         *
         * Advances the view to the next entity and returns a reference to the
         * updated view. This operation uses the derived class's increment logic
         * through CRTP to ensure proper filtering behavior.
         *
         * @return Derived& Reference to this view after advancement
         */
        Derived& operator++() { static_cast<Derived&>(*this).increment(); return static_cast<Derived&>(*this); }

        /**
         * @brief Post-increment operator that advances the view.
         *
         * Advances the view to the next entity and returns a copy of the view's
         * state before the increment. This supports traditional iterator usage
         * patterns where the pre-increment value is needed.
         *
         * @return Derived A copy of the view before increment
         */
        Derived operator++(int) { auto tmp = *this; static_cast<Derived&>(*this).increment(); return tmp; }

        /**
         * @brief Returns this view as a begin iterator for range-based for loops.
         *
         * Enables the view to be used directly in range-based for loops by
         * returning itself as the begin iterator. This creates a consistent
         * interface where the view object itself represents the iteration range.
         *
         * @return Derived Reference to this view as the begin iterator
         */
        [[nodiscard]] Derived begin() noexcept { return static_cast<Derived&>(*this); }

        /**
         * @brief Returns an end marker iterator for termination conditions.
         *
         * Provides a sentinel iterator representing the end of the entity collection.
         * This allows range-based for loops to determine when iteration is complete.
         *
         * @return EntitiesIterator Empty iterator representing the end position
         */
        [[nodiscard]] EntitiesIterator end() const noexcept { return {}; }

    protected:
        EntitiesIterator m_iterator;                    ///< Underlying iterator tracking current position
        EntitiesManager& m_manager;                     ///< Reference to entities manager for data access
    };


    /**
     * @brief View that provides Entity objects for each entity in the iteration.
     *
     * This view transforms the underlying entity IDs into full Entity objects
     * complete with component access. When dereferenced, it constructs an Entity
     * that combines the ID with a reference to its associated components, providing
     * a unified interface for entity manipulation during iteration.
     */
    struct EntityViews : DerivedViews<EntityViews>
    {
        using DerivedViews::DerivedViews;

        /**
         * @brief Dereference operator that returns an Entity for the current position.
         *
         * Constructs and returns a complete Entity object by combining the current
         * entity ID with a reference to its components retrieved from the manager.
         * This enables iteration over full entity objects rather than just IDs.
         *
         * @return Entity An entity object containing the ID and component reference
         */
        Entity operator*() const { const entityId_t id = *m_iterator; return {.id=id, .components=*m_manager.find(id)}; }
    };


    /**
     * @brief View that filters entities based on component requirements.
     *
     * This template provides iteration over entities that contain all specified
     * component types. It automatically skips entities that don't have the required
     * components and provides direct access to those components when dereferenced.
     * The view can also iterate over entities regardless of their components when
     * instantiated with an empty template parameter pack.
     *
     * @tparam ComponentCls The component types that entities must possess
     */
    template<typename... ComponentCls>
    struct ComponentsViews : DerivedViews<ComponentsViews<ComponentCls...>>
    {
    private:
        using Super = DerivedViews<ComponentsViews<ComponentCls...>>;
        static constexpr bool IsZeroComponents = sizeof...(ComponentCls) == 0;

    public:
        /**
         * @brief Constructs a component-filtered view starting from a specific iterator.
         *
         * Initializes the view and automatically advances to the first entity that
         * contains all required component types. If no components are specified,
         * it starts from the given iterator position without filtering.
         *
         * @param iterator The starting iterator position
         * @param manager Reference to the entities manager for component access
         */
        ComponentsViews(EntitiesIterator&& iterator, EntitiesManager& manager);

        /**
         * @brief Dereference operator providing access to entity components.
         *
         * Returns either a reference to the full Components object (when no component
         * types are specified) or a tuple of references to the requested component
         * types. This provides flexible component access based on the view's template
         * parameters.
         *
         * @return Conditional type: Components& or std::tuple<ComponentCls&...>
         */
        std::conditional_t<IsZeroComponents, Components&, std::tuple<ComponentCls&...>> operator*();

        /**
         * @brief Advances the iterator to the next entity matching component criteria.
         *
         * When filtering by component types, this method skips entities that don't
         * contain all required components. For zero-component views, it simply
         * advances to the next entity without filtering. The iteration stops when
         * reaching the end of the entity collection.
         */
        void increment();
    };



} // namespace ecs::ranges::view

#include "detail/EntitiesViews.ipp"
