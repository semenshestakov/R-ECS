#pragma once
#include "Component.hpp"


namespace ecs::component
{

    class DynamicComponentsFactory final
    {
    public:
        DynamicComponentsFactory();
        explicit DynamicComponentsFactory(bufferSize_t bufferSize);
        ~DynamicComponentsFactory() noexcept;

        DynamicComponentsFactory(DynamicComponentsFactory&& a_other) noexcept;
        DynamicComponentsFactory& operator=(DynamicComponentsFactory&& a_other) noexcept;
        void swap(DynamicComponentsFactory& a_other) noexcept;

        DynamicComponentsFactory(const DynamicComponentsFactory&) = delete;
        DynamicComponentsFactory& operator=(const DynamicComponentsFactory&) = delete;

        template<class CLASS, typename... Args> void add(Args&&... args);
        template<class CLASS> CLASS* get();

    private:
        struct StackHead
        {
            componentId_t componentTypeId = INVALID_COMPONENT_ID;
            bufferSize_t componentSize = 0;

            [[nodiscard]] inline StackHead* next() { return reinterpret_cast<StackHead*>(reinterpret_cast<byte*>(this) + sizeof(StackHead) + componentSize); }
        };
        static constexpr bufferSize_t s_sizeOfStackHead = sizeof(StackHead);

        bufferSize_t m_bufferCapacity = {};
        byte* m_buffer = nullptr;

        bool resize(bufferSize_t a_size) noexcept;
        [[nodiscard]] StackHead* getLastHead() const noexcept;
        [[nodiscard]] StackHead* findStackHeadById(componentId_t a_id) const noexcept;
        template<class CLASS> constexpr static componentId_t getComponentsTypeId();

    };

}

#include "detail/DynamicComponentsFactory.inl"
