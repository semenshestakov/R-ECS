#pragma once
#include "Component.hpp"


namespace ecs::component
{

    class ComponentsFactory final
    {
    public:
        ComponentsFactory();
        explicit ComponentsFactory(bufferSize_t bufferSize);
        ~ComponentsFactory() noexcept;

        ComponentsFactory(ComponentsFactory&& a_other) noexcept;
        ComponentsFactory& operator=(ComponentsFactory&& a_other) noexcept;
        void swap(ComponentsFactory& a_other) noexcept;

        ComponentsFactory(const ComponentsFactory&) = delete;
        ComponentsFactory& operator=(const ComponentsFactory&) = delete;

        template<class CLASS, typename... Args> void add(Args&&... args);
        template<class CLASS> CLASS* get();

    private:
        struct StackHead
        {
            componentId_t componentTypeId = INVALID_COMPONENT_ID;
            bufferSize_t componentSize = 0;
            void(*destructor)(void*) = nullptr;

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

#include "detail/ComponentsFactory.inl"