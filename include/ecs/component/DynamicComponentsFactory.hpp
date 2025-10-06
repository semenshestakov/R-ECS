/**
 * @file DynamicComponentsFactory.hpp
 * @brief Factory for managing dynamic component allocation for single factory
 *
 * Provides memory management for dynamically allocated components within
 * an Entity Component System. Uses a custom stack-based allocation strategy
 * with automatic resizing and proper destruction of components.
 */
#pragma once
#include "Component.hpp"


namespace ecs::component
{

    class DynamicComponentsFactory final
    {
    public:
        /**
         * @brief Default constructor
         *
         * Creates a factory with zero initial buffer capacity.
         * Buffer will be allocated on first component addition.
         */
        DynamicComponentsFactory();

        /**
         * @brief Constructor with specified buffer size
         * @param bufferSize Initial capacity of the component buffer in bytes
         *
         * Allocates a buffer of the specified size if non-zero.
         * Minimum buffer size is guaranteed to be at least 2 * s_sizeOfStackHead.
         */
        explicit DynamicComponentsFactory(bufferSize_t bufferSize);

        /**
         * @brief Destructor
         *
         * Properly destroys all allocated components and deletes the buffer.
         * Calls destructors for all components in reverse order of allocation.
         */
        ~DynamicComponentsFactory() noexcept;

        /**
         * @brief Move constructor
         * @param other Factory to move from
         *
         * Transfers ownership of the buffer from another factory.
         * The source factory will be left in an empty state.
         */
        DynamicComponentsFactory(DynamicComponentsFactory&& other) noexcept;

        /**
         * @brief Move assignment operator
         * @param other Factory to move from
         * @return Reference to this factory
         *
         * Swaps contents with the source factory. The source factory
         * will contain the previous contents of this factory.
         */
        DynamicComponentsFactory& operator=(DynamicComponentsFactory&& other) noexcept;

        /**
         * @brief Swap contents with another factory
         * @param other Factory to swap with
         *
         * Efficiently exchanges buffer ownership between two factories.
         * No memory allocation or component copying occurs.
         */
        void swap(DynamicComponentsFactory& other) noexcept;

        // delete copy
        DynamicComponentsFactory(const DynamicComponentsFactory&) = delete;
        DynamicComponentsFactory& operator=(const DynamicComponentsFactory&) = delete;

        /**
         * @brief Add a new component to the factory
         * @tparam COMPONENT Component type to add (must inherit from BaseComponent)
         * @tparam Args Argument types for component constructor
         * @param args Arguments to forward to component constructor
         * @throw BaseComponentError if component already exists or allocation fails
         *
         * Constructs a new component in the factory's buffer. Automatically
         * resizes the buffer if necessary. Each component is prefixed with
         * a StackHead containing metadata.
         */
        template<BaseOfComponents COMPONENT, typename... Args> void add(Args&&... args);

        /**
         * @brief Get a component by type
         * @tparam COMPONENT Component type to retrieve
         * @return Pointer to the component, or nullptr if not found
         * @throw BaseComponentError if found component has insufficient size
         *
         * Looks up a component by its type ID. Returns nullptr if the
         * component doesn't exist in the factory.
         */
        template<BaseOfComponents COMPONENT> COMPONENT* get();

    private:
        /**
         * @brief Header structure for component memory management
         *
         * StackHead precedes each component in the memory buffer and contains
         * metadata needed for traversal, destruction, and management of components.
         * Forms a linked list structure within the contiguous memory buffer.
         */
        struct StackHead
        {
            componentId_t componentTypeId = INVALID_COMPONENT_ID;
            bufferSize_t componentSize = 0;

            [[nodiscard]] inline StackHead* next() { return reinterpret_cast<StackHead*>(reinterpret_cast<byte*>(this) + sizeof(StackHead) + componentSize); }
        };
        static constexpr bufferSize_t s_sizeOfStackHead = sizeof(StackHead);

        bufferSize_t m_bufferCapacity = {};
        byte* m_buffer = nullptr;

        /**
         * @brief Resize the internal buffer
         * @param a_size New buffer size in bytes
         * @return true if buffer was resized, false if new size is smaller than current
         *
         * Increases the buffer capacity while preserving existing components.
         * If resize occurs, all existing component pointers become invalid.
         */
        bool resize(bufferSize_t a_size) noexcept;

        /**
         * @brief Get the last stack head in the buffer
         * @return Pointer to the last StackHead, or nullptr if buffer is empty
         *
         * Traverses the linked list of stack heads to find the last one
         * (where componentSize == 0, indicating free space).
         */
        [[nodiscard]] StackHead* getLastHead() const noexcept;

        /**
         * @brief Find a stack head by component ID
         * @param componentId Component ID to search for
         * @return Pointer to the StackHead, or nullptr if not found
         *
         * Linear search through the buffer to find a component with
         * the specified type ID.
         */
        [[nodiscard]] StackHead* findStackHeadById(componentId_t componentId) const noexcept;

        /**
         * @brief Get the component type ID for a class
         * @tparam COMPONENT Component class type
         * @return Component type ID
         *
         * Static method that returns the compile-time component ID.
         * Validates that the class derives from BaseComponent and has
         * a valid component ID defined.
         */
        template<BaseOfComponents COMPONENT> constexpr static componentId_t getComponentsTypeId();

    };

}

#include "detail/DynamicComponentsFactory.inl"
