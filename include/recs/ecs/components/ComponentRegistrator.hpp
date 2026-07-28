#ifndef COMPONENTS_REGISTRATOR_HPP
#define COMPONENTS_REGISTRATOR_HPP

#include <mutex>
#include "Utils.hpp"
#include "collections/BitSet.hpp"
#include "reg/Registrator.hpp"


namespace ecs
{

    /**
     * @brief Metadata for component registration and construction.
     */
    struct RegisterComponentInfo final
    {
        std::string name;                                           ///< Raw (mangled) type name from typeid
        std::string dname;                                          ///< Demangled, human-readable type name
        bufferSize_t componentSize {0};                             ///< Size of the component in bytes (0 for tags)
        componentId_t componentId {INVALID_COMPONENT_ID};           ///< Unique identifier for the component
        bool isTag {false};                                         ///< True for zero-sized archetype tags (no chunk column)
        void(*constructor)(byte*) = nullptr;                        ///< Placement new constructor function
        void(*destructor)(byte*) = nullptr;                         ///< Destructor function
        void(*copy)(byte* to, byte* from) = nullptr;                ///< Copy function
        void(*move)(byte* to, byte* from) = nullptr;                ///< Move function
        byte*(*poolAcquire)() = nullptr;                            ///< Acquire one component-sized slot from the free list
        void(*poolRelease)(byte*) = nullptr;                        ///< Return slot to the free list

        template <IsComponent ComponentCls>
        static RegisterComponentInfo Create(const std::string& name);
    };

    /**
     * @brief Component registration manager for the ECS framework.
     *
     * The ComponentRegistrator class provides static methods to register component types
     * with the ECS system. It manages the assignment of unique component IDs and maintains
     * registration information for each component type.
     *
     * @note This class uses protected inheritance from reg::Registrator to extend
     *       registration functionality while maintaining encapsulation.
     * @note Component IDs start from 1 (0 is reserved for invalid/unregistered components).
     *
     * @tparam ComponentCls The component type to register.
     */
    class ComponentRegistrator final : protected reg::Registrator<RegisterComponentInfo, reg::RegistrationStrategy::UNIQUE>
    {
        using Super = reg::Registrator<RegisterComponentInfo, reg::RegistrationStrategy::UNIQUE>;
    public:

        ComponentRegistrator() = delete;

        using Super::size;
        using Super::iter;

        /**
         * @brief Registers a component type and assigns it a unique ID.
         *
         * Registers the specified component type with the ECS system. Each registered
         * component receives a unique numeric identifier that can be used throughout
         * the ECS framework to reference this component type.
         *
         * The registration process:
         * 1. Creates a registration entry for the component type
         * 2. Assigns a unique component ID (index + 1)
         * 3. Stores the ID in the registration metadata
         * 4. Returns the assigned ID for immediate use
         *
         * @tparam ComponentCls Type of component to register
         *
         * @return componentId_t Unique ID assigned to this component type
         *
         * @note Component IDs start from 1 (0 is reserved for invalid/unregistered)
         * @note Registration is idempotent - calling multiple times returns the same ID
         */
        template<IsComponent ComponentCls>
        static componentId_t Register();

        /**
         * @brief Retrieves registration information for a component by its ID.
         *
         * Returns a const reference to the registration metadata associated with the
         * specified component ID. The returned information includes the component's
         * name, size, constructor, destructor, and other metadata registered during
         * the component registration process.
         *
         * This method is used internally by the ECS framework to obtain component
         * information needed for memory management, serialization, and runtime
         * type operations.
         *
         * @param componentId Unique identifier of the component (obtained from Register())
         *
         * @return const RegisterComponentInfo& Constant reference to the registration
         *         information structure for the specified component
         *
         * @throw std::out_of_range If the provided componentId is invalid or
         *         not registered in the system
         *
         * @note The returned reference remains valid for the lifetime of the program
         * @note Component ID 0 (INVALID_COMPONENT_ID) will throw an exception
         *
         * @see Register() for obtaining component IDs
         * @see RegisterComponentInfo for the structure of registration information
         */
        static const RegisterComponentInfo& GetInfo(componentId_t componentId);

        /**
         * @brief Retrieves the component ID for a given component type.
         *
         * Template function that returns the unique `componentId_t` corresponding
         * to the provided component class. The component class must have been
         * registered with the ECS system beforehand.
         *
         * @tparam ComponentCls Component type to get the ID for.
         *
         * @return componentId_t Unique ID of the component type.
         *
         * @note If the component type was not registered, behavior is undefined.
         */
        template<IsComponent ComponentCls>
        static componentId_t GetComponentId();

        /**
         * @brief Returns the global mask of every registered tag's componentId bit.
         *
         * Bit `id` is set iff the component registered under `id` is a tag
         * (zero-sized, `IsTag`). Grows automatically as new tag types register.
         * Intersecting an entity's archetype with this mask isolates its tag bits,
         * e.g. `EntitiesManager::GetTagIds`.
         *
         * @return Const reference to the process-wide tags mask.
         */
        [[nodiscard]] static const collections::BitSet& GetTagsMask();

        /**
         * @brief Returns the global mask of every registered (non-tag) component's
         * componentId bit.
         *
         * Bit `id` is set iff the component registered under `id` carries data
         * (`!IsTag`). Grows automatically as new component types register.
         * Intersecting an entity's archetype with this mask isolates its data-bearing
         * component bits, e.g. `EntitiesManager::GetComponentIds`.
         *
         * @return Const reference to the process-wide components mask.
         */
        [[nodiscard]] static const collections::BitSet& GetComponentsMask();

    private:
        /// Serializes first-time registration of distinct component types, which
        /// mutates the shared collection. The per-type magic static in
        /// GetComponentId() guarantees Register() runs at most once per type; this
        /// mutex guards two *different* types registering concurrently (e.g. both
        /// first touched inside a parallel view iteration).
        inline static std::mutex s_registrationMutex;

        /// Bit `id` set for every registered tag componentId. Populated in Register()
        /// under s_registrationMutex; see GetTagsMask().
        inline static collections::BitSet s_tagsMask;

        /// Bit `id` set for every registered non-tag componentId. Populated in
        /// Register() under s_registrationMutex; see GetComponentsMask().
        inline static collections::BitSet s_componentsMask;
    };

}
#endif
#include "detail/ComponentRegistrator.ipp"
