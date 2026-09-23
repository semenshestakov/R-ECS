#pragma once
#include <unordered_map>
#include <memory>


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
     * are managed via std::unique_ptr, ensuring automatic cleanup when the SingletonStore is
     * destroyed.
     *
     * Key features:
     * - Type-safe access: retrieve instances by their type with compile-time guarantees
     * - Automatic construction: instances are created on-demand when accessed
     * - Move-only semantics: prevents accidental copying of unique resources
     * - Thread-unsafe: designed for single-threaded use (no internal synchronization)
     *
     * The SingletonStore is particularly useful in ECS architectures for storing system-wide
     * resources, configuration objects, and shared services that need to be accessed
     * across different parts of the application without passing references explicitly.
     *
     * @note This class is non-copyable but movable to support efficient transfer of ownership.
     * @warning The container does not provide thread safety; external synchronization is
     *          required for concurrent access from multiple threads.
     *
     */
    class SingletonStore
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
         * Automatically destroys all stored instances through their respective unique_ptr
         * deleters. Each instance is properly destructed in the order of storage.
         */
        ~SingletonStore() = default;

        /**
         * @brief Deleted copy constructor.
         *
         * Copying is disabled because std::unique_ptr members are non-copyable.
         * This prevents accidental duplication of unique resources.
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
         * arguments. If an instance of type T already exists, the behavior is
         * implementation-defined (typically the new instance overwrites the old one).
         *
         * @tparam T The type of object to construct.
         * @tparam Args The types of arguments to forward to T's constructor.
         * @param args The arguments to forward to T's constructor.
         * @return Reference to the newly constructed instance.
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
         * This method provides lazy initialization semantics.
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
         * If no instance exists, the operation does nothing (no error).
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

        using ctxId_t = std::size_t;        ///< Type alias for type identification keys

        /**
         * @brief Generates a unique identifier for type T.
         * @tparam T The type to generate an identifier for.
         * @return A hash code that uniquely identifies type T.
         */
        template<typename T>
        static constexpr ctxId_t getStoreKey();

        /// Storage container mapping type IDs to their instances
        std::unordered_map<ctxId_t, std::unique_ptr<BaseHolder>> m_data;
    };

} // namespace collections

#include "detail/SingletonStore.ipp"
