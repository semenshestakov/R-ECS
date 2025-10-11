#pragma once
#include <memory>
#include "Component.hpp"


namespace ecs::component
{

    /**
     * @brief Container for statically allocated ECS components with runtime initialization.
     *
     * Calculates total buffer size as:
     *   sizeof(StaticComponents) + (maxComponentId + 1) * sizeof(ComponentInfo)   // Metadata array
     *   + classBufferSize                              // Component data storage
     *
     * StaticComponents manages a fixed set of components allocated in a contiguous memory buffer.
     * It provides type-safe access to components while supporting conditional initialization
     * and lifetime management.
     *
     * @note The buffer layout enables O(1) component lookup by ID.
     * @note Move constructible/assignable but not copyable due to owned buffer memory.
     */
    class StaticComponents
    {
        /**
         * @brief Metadata describing the component storage layout and capacity.
         */
        struct ComponentsInfo
        {
            bufferSize_t maxComponentId {};       ///< Highest component ID in the container
        };

        /// @brief Container metadata including component ID range and storage information.
        ComponentsInfo m_componentsInfo;

        /**
        * @brief Constructs StaticComponents with the specified component layout.
        * @param componentsInfo Component container metadata and capacity.
        *
        * @note Takes ownership of the buffer - will delete it on destruction.
        */
        StaticComponents(const ComponentsInfo& componentsInfo);

    public:
        StaticComponents();

        /**
         * @brief Destroys the container and all initialized components.
         */
        ~StaticComponents();

        // Move semantics
        StaticComponents(StaticComponents&& other) noexcept = delete;
        StaticComponents& operator=(StaticComponents&& other) noexcept = delete;

        // Delete copy semantics
        StaticComponents(const StaticComponents&) = delete;
        StaticComponents& operator=(const StaticComponents&) = delete;

        /**
         * @brief Initializes a specific component with constructor arguments.
         *
         * @tparam COMPONENT Type of component to initialize.
         * @tparam Args Argument types for component constructor.
         * @param args Arguments to forward to component constructor.
         * @return true if component was found and initialized successfully.
         * @return false if component was not found.
         * @throws error::InvalidSizeComponents if component size mismatch detected.
         *
         * @note Zero-initializes memory before construction.
         * @note Properly destroys previously initialized component if reinitializing.
         */
        template<BaseOfComponents COMPONENT, typename ... Args> bool init(const Args&... args);

        /**
         * @brief Initializes all components using their default constructors.
         *
         * @note Only initializes components that haven't been initialized yet.
         */
        void initialize();

        /**
         * @brief Retrieves a pointer to a component of the specified type.
         *
         * @tparam COMPONENT Type of component to retrieve.
         * @return COMPONENT* Pointer to the component, or nullptr if not found/uninitialized.
         *
         * @note Returns nullptr if component is not initialized or size mismatch occurs.
         */
        template<BaseOfComponents COMPONENT> const COMPONENT* get() const;

        /**
         * @brief Retrieves a pointer to a component of the specified type with enforced existence.
         *
         * @tparam COMPONENT Type of component to retrieve.
         * @return const COMPONENT* Pointer to the component.
         *
         * @throws If the component is not found, uninitialized, or has size mismatch.
         * @note This is a safe alternative to get() when the component's presence is required.
         */
        template<BaseOfComponents COMPONENT> const COMPONENT* mustGet() const;

    private:

        /**
         * @brief Metadata for component registration and construction.
         */
        struct RegisterComponentInfo
        {
            bufferSize_t componentSize {0};                             ///< Size of the component in bytes
            componentId_t componentId {INVALID_COMPONENT_ID};           ///< Unique identifier for the component
            BaseComponent::conditionFunction_t condition {nullptr};     ///< Runtime condition check function
            void(*constructor)(byte*) = nullptr;                        ///< Placement new constructor function
        };

        /**
         * @brief Runtime information for a component instance.
         */
        struct ComponentInfo
        {
            byte* ptr = nullptr;                                            ///< Pointer to component data
            bool initialized {false};                                       ///< Whether component has been constructed
            const RegisterComponentInfo* registerComponentInfo = nullptr;   ///< Registration metadata

            /**
             * @brief Gets the component ID from registration info.
             * @return componentId_t Component ID or INVALID_COMPONENT_ID if no registration.
             */
            [[nodiscard]] componentId_t componentId() const { return registerComponentInfo ? registerComponentInfo->componentId : INVALID_COMPONENT_ID; }

            /**
             * @brief Gets the component size from registration info.
             * @return bufferSize_t Component size in bytes or 0 if no registration.
             */
            [[nodiscard]] bufferSize_t componentSize() const { return registerComponentInfo ? registerComponentInfo->componentSize : 0; }
        };

        /**
         * @brief Get component metadata by ID.
         * @return ComponentInfo* or nullptr if not found.
         */
        [[nodiscard]] ComponentInfo* getComponentInfoByComponentId(componentId_t componentId) const;

        /**
         * @brief Get component metadata by type.
         * @return ComponentInfo* or nullptr if not found.
         */
        template<BaseOfComponents COMPONENT> [[nodiscard]] ComponentInfo* getComponentInfoByComponent() const;

        /**
         * @brief Get iterator to first ComponentInfo.
         */
        [[nodiscard]] ComponentInfo* beginComponentInfo() const;

        /**
         * @brief Get iterator past last ComponentInfo.
         */
        [[nodiscard]] ComponentInfo* endComponentInfo() const;

        /**
         * @brief Get start data.
         */
        [[nodiscard]] byte* data() const;


        /**
         * @brief Allocates a contiguous buffer for both metadata and component storage.
         *
         * @param classBufferSize Total bytes needed for all component instances.
         * @param maxComponentId Highest component ID, determines metadata array size.
         * @return byte* Newly allocated buffer, or throws std::bad_alloc on failure.
         *
         * @note The buffer layout enables O(1) component lookup by ID.
         * @note Buffer must be deleted[] by the owner (StaticComponents destructor).
         * @warning classBufferSize must account for alignment requirements of all components.
         */
        static byte* newBuffer(bufferSize_t classBufferSize, componentId_t maxComponentId);

        friend class StaticComponentsFactory;
    };

    using StaticComponentsPtr= std::unique_ptr<StaticComponents>;

} // namespace ecs::component

#include "detail/StaticComponents.inl"
