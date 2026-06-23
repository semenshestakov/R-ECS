#pragma once
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>


namespace ecs
{

    /**
     * @brief Lightweight, non-owning reference to any callable.
     *
     * FunctionRef stores a pointer to an existing callable plus a thunk that
     * invokes it. It performs no heap allocation and does not extend the
     * lifetime of the referenced callable, so it is only valid while that
     * callable is alive. This makes it the right type for the body of a tight
     * loop such as IJobScheduler::ParallelFor, where the callable is always
     * live on the caller's stack and std::function's allocation overhead would
     * be unacceptable.
     *
     * Modelled after std::function_ref (which only lands in C++26). Unlike
     * std::function it is move/copy-trivial and never owns.
     *
     * @tparam Signature A function signature of the form R(Args...).
     */
    template <typename Signature>
    class FunctionRef;

    template <typename R, typename... Args>
    class FunctionRef<R(Args...)>
    {
    public:
        FunctionRef() = default;

        /**
         * @brief Binds the reference to an existing callable.
         * @tparam F Callable type invocable as R(Args...).
         * @param callable The callable to reference. Must outlive this FunctionRef.
         */
        template <typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, FunctionRef> && std::is_invocable_r_v<R, F&, Args...>>>
        FunctionRef(F&& callable) noexcept :
            m_obj(const_cast<void*>(static_cast<const void*>(std::addressof(callable)))),
            m_thunk([](void* obj, Args... args) -> R {
                return (*static_cast<std::remove_reference_t<F>*>(obj))(std::forward<Args>(args)...);
            })
        {
        }

        /**
         * @brief Invokes the referenced callable.
         */
        R operator()(Args... args) const { return m_thunk(m_obj, std::forward<Args>(args)...); }

        /**
         * @brief Whether the reference is bound to a callable.
         */
        [[nodiscard]] explicit operator bool() const noexcept { return m_thunk != nullptr; }

    private:
        void* m_obj = nullptr;                  ///< Erased pointer to the referenced callable
        R (*m_thunk)(void*, Args...) = nullptr; ///< Thunk that casts back and invokes
    };

} // namespace ecs
