#pragma once
#include "../ComponentFreeList.hpp"


template <ecs::IsComponent ComponentCls>
ecs::byte* ecs::ComponentFreeList<ComponentCls>::acquire()
{
    Node* head = s_head.load(std::memory_order_acquire);
    while (head)
    {
        if (s_head.compare_exchange_weak(head, head->next,
                std::memory_order_release,
                std::memory_order_acquire))
            return head->storage;
    }

    return std::bit_cast<byte*>(new Node);
}

template <ecs::IsComponent ComponentCls>
void ecs::ComponentFreeList<ComponentCls>::release(byte* ptr)
{
    auto* node = reinterpret_cast<Node*>(ptr);
    node->next = s_head.load(std::memory_order_relaxed);
    while (!s_head.compare_exchange_weak(node->next, node,
            std::memory_order_release,
            std::memory_order_relaxed))
    {}
}