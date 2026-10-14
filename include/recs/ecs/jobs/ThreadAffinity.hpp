#pragma once


namespace ecs
{

    /**
     * @brief Records the calling thread as the main (structural-mutation) thread.
     *
     * Registry calls this from its constructor. Until it runs, OnMainThread()
     * reports true so the affinity checks never fire before a Registry exists
     * (static init, unit tests that touch managers directly, etc.).
     */
    void MarkMainThread() noexcept;

    /// @brief True if no main thread is recorded yet, or the caller is that thread.
    [[nodiscard]] bool OnMainThread() noexcept;

#ifndef NDEBUG
    /**
     * @brief Debug-only check that a structural change runs on the main thread.
     * @param op A string literal naming the operation, used in the diagnostic.
     * @note Use through the ECS_ASSERT_MAIN_THREAD macro rather than directly.
     */
    void AssertMainThread(const char* op) noexcept;
#endif

} // namespace ecs


/**
 * @brief Asserts (debug builds only) that a structural change runs on the main thread.
 *
 * Entity and system lifetime operations (create/destroy entities, add/remove
 * components, register systems), event dispatch and context mutation must not be
 * issued from worker threads while systems run in parallel; defer them through
 * the command queue instead. Compiles to nothing under NDEBUG.
 *
 * @param op A string literal naming the operation.
 */
#ifndef NDEBUG
    #define ECS_ASSERT_MAIN_THREAD(op) ::ecs::AssertMainThread(op)
#else
    #define ECS_ASSERT_MAIN_THREAD(op) ((void) 0)
#endif
