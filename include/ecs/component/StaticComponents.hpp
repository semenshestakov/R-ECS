#pragma once
#include "Component.hpp"


namespace ecs::component
{

    class StaticComponents
    {
    public:
        StaticComponents();
        StaticComponents(bufferSize_t classBufferSize, componentId_t maxComponentId);
        ~StaticComponents();

        StaticComponents(StaticComponents&& other) noexcept;
        StaticComponents& operator=(StaticComponents&& other) noexcept;

        // delete copy
        StaticComponents(const StaticComponents&) = delete;
        StaticComponents& operator=(const StaticComponents&) = delete;

        template<BaseOfComponents COMPONENT, typename ... Args> bool init(const Args&...);
        void initialize();
        template<BaseOfComponents COMPONENT> COMPONENT* get() const;

    private:
        struct RegisterComponentInfo
        {
            bufferSize_t componentSize {0};
            componentId_t componentId {INVALID_COMPONENT_ID};
            BaseComponent::conditionFunction_t condition {nullptr};
            void(*constructor)(byte*) = nullptr;
        };
        static RegisterComponentInfo INVALID_REGISTER_COMPONENT_INFO;

        struct ComponentInfo
        {
            byte* ptr = nullptr;
            bool initialized {false};
            const RegisterComponentInfo* registerComponentInfo = &INVALID_REGISTER_COMPONENT_INFO;

            [[nodiscard]] componentId_t componentId() const { return registerComponentInfo->componentId; }
            [[nodiscard]] bufferSize_t componentSize() const { return registerComponentInfo->componentId; };
        };

        byte* m_buffer = nullptr;
        bufferSize_t m_maxComponentId {};

        [[nodiscard]] ComponentInfo* getComponentInfoByComponentId(componentId_t componentId) const;
        template<BaseOfComponents COMPONENT> [[nodiscard]] ComponentInfo* getComponentInfoByComponent() const;
        [[nodiscard]] byte* getComponentsDataPtr() const;
        [[nodiscard]] ComponentInfo* beginComponentInfo() const;
        [[nodiscard]] ComponentInfo* endComponentInfo() const;

        void swap(StaticComponents& other) noexcept;

        friend class StaticComponentsFactory;
    };

} // namespace ecs::component

#include "detail/StaticComponents.inl"
