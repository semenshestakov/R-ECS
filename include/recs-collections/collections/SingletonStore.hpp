#pragma once
#include <unordered_map>
#include <memory>
#include <functional>

#include "AllocatorPolicy.hpp"
#include "MutexPolicy.hpp"


namespace collections
{

    /**
     * @brief A type-safe dependency injection container for managing singleton instances.
     *
     * The SingletonStore class provides a simple yet powerful mechanism for storing and retrieving
     * singleton instances of any type. It uses type-based indexing to ensure type safety
     * and automatic memory management through RAII principles. Each stored instance is
     * uniquely identified by its type, and the container ensures that only one instance
     * of each type exists at any given time.
     *
     * The class employs a type-erasure technique with a polymorphic base class to store
     * instances of arbitrary types while maintaining type safety. This allows the container
     * to hold heterogeneous types in a single homogeneous container. All stored objects
     * are managed via a type-erased smart pointer, ensuring automatic cleanup when the
     * SingletonStore is destroyed.
     *
     * Key features:
     * - Type-safe access: retrieve instances by their type with compile-time guarantees
     * - Automatic construction: instances are created on-demand when accessed
     * - Move-only semantics: prevents accidental copying of unique resources
     * - Opt-in thread safety: pass a real mutex type as @p Mutex to guard every operation
     *   with it; the default (`void`) disables locking entirely, at no runtime cost
     * - Opt-in custom allocation: pass an allocator type as @p Alloc to allocate/construct/
     *   destroy/deallocate stored instances through it; the default (`void`) uses plain new/delete
     *
     * The SingletonStore is particularly useful in ECS architectures for storing system-wide
     * resources, configuration objects, and shared services that need to be accessed
     * across different parts of the application without passing references explicitly.
     *
     * @tparam Alloc Allocator type used to allocate stored instances, or `void` (default)
     *         to use plain new/delete. See AllocatorPolicy.
     * @tparam Mutex Mutex type used to synchronize access, or `void` (default) to disable
     *         internal locking. See MutexPolicy.
     *
     * @note This class is non-copyable but movable to support efficient transfer of ownership.
     * @warning With the default template arguments no synchronization is performed; pass a
     *          mutex type as @p Mutex to make a given SingletonStore instance safe for
     *          concurrent access from multiple threads.
     *
     */
    template<typename Alloc = void, typename Mutex = void>
    class SingletonStore : protected AllocatorPolicy<Alloc>, protected MutexPolicy<Mutex>
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Creates an empty SingletonStore with no stored instances. The container starts in a
         * valid empty state and can be populated later through emplace operations.
         */
        SingletonStore() = default;

        /**
         * @brief Destructor.
         *
         * Automatically destroys all stored instances through their respective deleters
         * (which route through @p Alloc when one is configured). Each instance is properly
         * destructed in the order of storage.
         */
        ~SingletonStore() = default;

        /**
         * @brief Deleted copy constructor.
         *
         * Copying is disabled because the stored instances are held through a move-only
         * smart pointer. This prevents accidental duplication of unique resources.
         */
        SingletonStore(const SingletonStore&) = delete;

        /**
         * @brief Deleted copy assignment operator.
         *
         * Copy assignment is disabled to maintain unique ownership semantics of
         * the stored instances and prevent resource duplication.
         */
        SingletonStore& operator=(const SingletonStore&) = delete;

        /**
         * @brief Move constructor.
         *
         * Transfers ownership of all stored instances from another SingletonStore.
         * The source SingletonStore is left in a valid but unspecified state.
         *
         * @param other The SingletonStore to move from.
         */
        SingletonStore(SingletonStore&& other) noexcept = default;

        /**
         * @brief Move assignment operator.
         *
         * Transfers ownership of all stored instances from another SingletonStore,
         * destroying any previously stored instances in this SingletonStore.
         *
         * @param other The SingletonStore to move from.
         * @return Reference to this SingletonStore after the move.
         */
        SingletonStore& operator=(SingletonStore&& other) noexcept = default;

        /**
         * @brief Constructs and stores a new instance of type T.
         *
         * Creates a new instance of type T using perfect forwarding of the provided
         * arguments. If an instance of type T already exists, it is replaced: the new
         * instance is constructed, then the previous one is destroyed (any reference
         * to it is invalidated). When @p Mutex is configured, the whole operation is
         * performed under lock.
         *
         * @tparam T The type of object to construct.
         * @tparam Args The types of arguments to forward to T's constructor.
         * @param args The arguments to forward to T's constructor.
         * @return Reference to the newly constructed instance.
         *
         * @warning The mutex (when configured) only protects the container's internal
         *          bookkeeping, not the lifetime of a previously returned reference: if
         *          multiple threads call emplace<T>() for the *same* T concurrently,
         *          each call still invalidates the instance any other thread may be
         *          holding a reference to. Prefer creating shared instances up front on
         *          one thread (e.g. during Init) and only reading them concurrently
         *          afterwards with get<T>()/has<T>(), as ecs::Context's docs recommend.
         *
         * @example
         * @code
         * struct MyClass {
         *     MyClass(int a, std::string b) : x(a), y(b) {}
         *     int x;
         *     std::string y;
         * };
         *
         * ecs::SingletonStore store;
         * auto& obj = store.emplace<MyClass>(42, "hello");
         * assert(obj.x == 42 && obj.y == "hello");
         * @endcode
         */
        template<typename T, typename... Args>
        T& emplace(Args&&... args);

        /**
         * @brief Retrieves a reference to the stored instance of type T.
         *
         * Accesses the previously stored instance of type T. The behavior is undefined
         * if no instance of type T exists in the SingletonStore (assertion will trigger in debug builds).
         * This method is more efficient than getOrEmplace() as it doesn't check for existence.
         * When @p Mutex is configured, the lookup is performed under lock.
         *
         * @tparam T The type of instance to retrieve.
         * @return Reference to the stored instance of type T.
         * @throws Assertion failure if the type does not exist (in debug builds).
         *
         * @warning Always check with has<T>() before calling get<T>() if existence is uncertain.
         *
         * @example
         * @code
         * store.emplace<std::string>("initialized");
         * if (store.has<std::string>()) {
         *     auto& str = store.get<std::string>();
         *     str += " - modified";
         * }
         * @endcode
         */
        template<typename T>
        [[nodiscard]] T& get();
        template<typename T>
        [[nodiscard]] const T& get() const;

        /**
         * @brief Retrieves an existing instance or constructs a new one.
         *
         * Attempts to retrieve an existing instance of type T. If no instance exists,
         * it constructs a new one using the provided arguments and stores it.
         * This method provides lazy initialization semantics. When @p Mutex is configured,
         * the check and the construction happen under a single, uninterrupted lock, so two
         * threads racing to lazily initialize the same type cannot both succeed.
         *
         * @tparam T The type of instance to retrieve or construct.
         * @tparam Args The types of arguments for construction (if needed).
         * @param args The arguments to forward to T's constructor (if construction is needed).
         * @return Reference to the existing or newly constructed instance.
         *
         * @example
         * @code
         * // First call constructs the instance
         * auto& logger = store.getOrEmplace<Logger>("app.log");
         *
         * // Second call returns the existing instance
         * auto& sameLogger = store.getOrEmplace<Logger>();
         * assert(&logger == &sameLogger);
         * @endcode
         */

        template<typename T, typename... Args>
        T& getOrEmplace(Args&&... args);

        /**
         * @brief Checks whether an instance of type T exists in the SingletonStore.
         *
         * Performs a lookup to determine if an instance of the specified type has been
         * stored in the SingletonStore. This operation is constant time on average.
         * When @p Mutex is configured, the lookup is performed under lock.
         *
         * @tparam T The type to check for existence.
         * @return true if an instance of type T exists, false otherwise.
         *
         * @example
         * @code
         * if (!store.has<DatabaseConnection>()) {
         *     auto& db = store.emplace<DatabaseConnection>("localhost", 5432);
         * }
         * @endcode
         */
        template<typename T>
        [[nodiscard]] bool has() const;

        /**
         * @brief Removes the stored instance of type T from the SingletonStore.
         *
         * Destroys the stored instance of type T and removes it from the container.
         * If no instance exists, the operation does nothing (no error). When @p Mutex
         * is configured, the removal is performed under lock.
         *
         * @tparam T The type of instance to remove.
         *
         * @example
         * @code
         * store.emplace<TemporaryResource>();
         * assert(store.has<TemporaryResource>());
         *
         * store.remove<TemporaryResource>();
         * assert(!store.has<TemporaryResource>());  // Instance is gone
         * @endcode
         */
        template<typename T>
        void remove();

    private:
        /**
         * @brief Abstract base class for type-erased storage.
         *
         * Provides a common interface for all stored instances regardless of their
         * actual type. The virtual destructor ensures proper cleanup of derived types.
         */
        struct BaseHolder
        {
            virtual ~BaseHolder() = default;
        };

        /**
         * @brief Concrete wrapper that stores the actual instance of type T.
         *
         * This template class is the actual storage unit for each stored instance.
         * It holds the value directly and provides access through the value member.
         * The class is final to prevent further inheritance and enable devirtualization.
         *
         * @tparam T The actual type of the stored instance.
         */
        template<typename T>
        struct Holder final : BaseHolder
        {
            T value;                        ///< The actual stored instance
            template<typename... Args>

            /**
             * @brief Constructs the invoker with the stored value.
             * @tparam Args The types of arguments to forward.
             * @param args Arguments to forward to T's constructor.
             */
            explicit Holder(Args... args) : value(std::forward<Args>(args)...) {}
        };

        /// Type-erased deleter: plain delete when @p Alloc is void, otherwise a deleter
        /// that routes destruction and deallocation back through the allocator that
        /// performed the original allocation.
        using Deleter = std::function<void(BaseHolder*)>;

        /// Owning pointer to a stored instance's type-erased holder.
        using HolderPtr = std::unique_ptr<BaseHolder, Deleter>;

        /**
         * @brief Allocates and constructs a Holder<T>, wiring up the matching deleter.
         *
         * When @p Alloc is configured, allocation/construction/destruction/deallocation
         * all go through it (rebound to Holder<T>); the allocator used is captured by
         * the deleter so the right allocator is used to free the memory later, even if
         * it is a stateful instance. When @p Alloc is void, this is plain new, freed by
         * plain delete.
         *
         * @tparam T The type to construct.
         * @tparam Args The types of arguments to forward to T's constructor.
         * @param args The arguments to forward to T's constructor.
         * @return An owning, type-erased pointer to the new holder.
         */
        template<typename T, typename... Args>
        HolderPtr makeHolder(Args&&... args);

        /**
         * @brief Core of emplace(), assuming the lock (if any) is already held.
         *
         * Exists so getOrEmplace() can perform its existence check and the construction
         * under one uninterrupted critical section instead of releasing and re-acquiring
         * the lock between them (which would reopen the race it exists to close).
         */
        template<typename T, typename... Args>
        T& emplaceImpl(Args&&... args);

        using ctxId_t = std::size_t;        ///< Type alias for type identification keys

        /**
         * @brief Generates a unique identifier for type T.
         * @tparam T The type to generate an identifier for.
         * @return A hash code that uniquely identifies type T.
         */
        template<typename T>
        static constexpr ctxId_t getStoreKey();

        /// Storage container mapping type IDs to their instances
        std::unordered_map<ctxId_t, HolderPtr> m_data;
    };

} // namespace collections

#include "detail/SingletonStore.ipp"
