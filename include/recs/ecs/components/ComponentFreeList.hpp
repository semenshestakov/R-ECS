#ifndef COMPONENT_FREE_LIST_HPP
#define COMPONENT_FREE_LIST_HPP

#include <atomic>
#include <cstddef>

#include "Utils.hpp"


namespace ecs
{

    /**
     * @brief Lock-free per-type free-list allocator for single component instances.
     *
     * Maintains a global singly-linked free list of fixed-size slots, each exactly
     * large enough to hold one instance of ComponentCls. Slots are recycled across
     * allocations, eliminating repeated calls to the system allocator on the entity
     * creation hot path.
     *
     * Thread safety: acquire() and release() are lock-free and safe to call
     * concurrently from multiple threads.
     *
     * @tparam ComponentCls Component type whose instances are pooled.
     *                      Must satisfy IsComponent.
     */
    template<IsComponent ComponentCls>
    class ComponentFreeList
    {
        /// Slot size: at least sizeof(void*) so the free-list pointer fits inside a recycled node.
        static constexpr std::size_t k_blockSize = sizeof(ComponentCls) < sizeof(void*) ? sizeof(void*) : sizeof(ComponentCls);

        /**
         * @brief Intrusive free-list node.
         *
         * When free, the node's `next` pointer links it into the free list.
         * When live, `storage` holds the constructed component bytes.
         * The two uses are mutually exclusive, so they share memory via a union.
         */
        union Node
        {
            Node* next;                                 ///< Next free node (valid only while the slot is in the free list).
            alignas(ComponentCls) byte storage[k_blockSize]; ///< Raw storage for one ComponentCls instance (valid while the slot is live).
        };

    public:
        ComponentFreeList() = delete;

        /**
         * @brief Acquires a raw memory slot sized for one ComponentCls instance.
         *
         * Pops the head node from the free list with a CAS loop. If the list is
         * empty, falls back to a heap allocation. The returned pointer is suitably
         * aligned for placement-new of ComponentCls.
         *
         * @return byte* Pointer to uninitialized storage; never null.
         */
        [[nodiscard]] static byte* acquire();

        /**
         * @brief Returns a previously acquired slot to the free list.
         *
         * Pushes the slot back onto the head of the free list with a CAS loop.
         * The caller is responsible for destroying the component object stored
         * in the slot before calling release().
         *
         * @param ptr Pointer previously returned by acquire().
         */
        static void release(byte* ptr);

    private:
        inline static std::atomic<Node*> s_head { nullptr };  ///< Head of the intrusive free list; nullptr when empty.
    };

} // namespace ecs
#endif
#include "detail/ComponentFreeList.ipp"
