#pragma once
#include <type_traits>


namespace collections
{

    /**
     * @brief Compile-time policy selecting whether a container uses a custom allocator.
     *
     * When @p Alloc is `void` (the default), the policy is a std::false_type and no
     * allocator is involved: the container falls back to plain new/delete. When
     * @p Alloc names an allocator type, the policy is a std::true_type and exposes
     * getAllocator() so the owning container can obtain an allocator to use.
     *
     * @tparam Alloc The allocator type to use, or void to disable custom allocation.
     */
    template<typename Alloc = void>
    struct AllocatorPolicy : std::true_type
    {
    protected:
        /**
         * @brief Returns an allocator to use.
         *
         * @p Alloc is stateless in this specialization, so a fresh, default-constructed
         * instance is handed out on every call; any two such instances are required to
         * compare equal, so it is safe to allocate with one copy and deallocate with another.
         */
        [[nodiscard]] static Alloc getAllocator()
        {
            return Alloc();
        }
    };

    template<>
    struct AllocatorPolicy<void> : std::false_type
    {
    };


}
