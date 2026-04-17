#pragma once
#include <ranges>
#include <string>
#include <vector>


namespace reg
{

    /**
     * @brief Basic registration info structure containing only a name.
     *
     * This is the default factory class used by Registrator when no custom
     * factory is provided. It stores only the registration name.
     */
    struct NameRegistration
    {
        const std::string name;     ///< Name associated with this registration

        /**
         * @brief Factory method to create a NameRegistration instance.
         *
         * @tparam T Type parameter (unused, maintained for interface compatibility)
         * @param name Name for the registration
         * @return NameRegistration instance
         */
        template <typename T>
        static NameRegistration Create(const std::string& name) { return {name}; }
    };

    /**
     * @brief Enumeration of registration strategies.
     *
     * Determines how duplicate registrations are handled by the Registrator.
     */
    enum class RegistrationStrategy : unsigned short
    {
        DEFAULT,
        UNIQUE,
    };

    /**
     * @brief A RAII-based registration manager for tracking named registrations.
     *
     * Registrator provides a mechanism to register and track objects with names
     * using either DEFAULT or UNIQUE registration strategies. The class uses
     * RAII semantics - registrations are automatically cleaned up when the
     * Registrator object is destroyed.
     *
     * @tparam FactoryCls The factory class used to create registration entries.
     *                    Must provide a static `create<T>(name)` method.
     *                    Defaults to NameRegistration.
     * @tparam Strategy Registration strategy (DEFAULT or UNIQUE).
     *                  Defaults to DEFAULT.
     *
     * @note This class is non-copyable but movable.
     * @note Inherit from this class to extend functionality while maintaining
     *       RAII registration management.
     *
     * @example
     * // Basic usage with default NameRegistration
     * auto reg1 = Registrator<>::Create<int>("MyComponent");
     * auto reg2 = Registrator<>::Create<float>("AnotherComponent");
     *
     * // Using UNIQUE strategy
     * auto uniqueReg = Registrator<NameRegistration, RegistrationStrategy::UNIQUE>::Create<double>("UniqueComponent");
     *
     * // Custom factory class
     * struct MyFactory {
     *     const std::string name;
     *     int data;
     *
     *     template<typename T>
     *     static MyFactory create(const std::string& name) {
     *         return {name, sizeof(T)};
     *     }
     * };
     *
     * auto customReg = Registrator<MyFactory>::Create<MyClass>("Custom");
     */
    template <typename FactoryCls = NameRegistration, RegistrationStrategy Strategy = RegistrationStrategy::DEFAULT>
    class Registrator
    {
    public:
        Registrator() = delete;                                         ///< No default construction
        ~Registrator() noexcept;                                        ///< Destructor cleans up registration

        Registrator(const Registrator& other) = delete;                 ///< Non-copyable
        Registrator& operator=(const Registrator& other) = delete;      ///< Non-copyable

        Registrator(Registrator&& other) noexcept;                      ///< Move constructor
        Registrator& operator=(Registrator&& other) noexcept;           ///< Move assignment

    protected:
        /**
         * @brief Type of items stored in the registration collection.
         *
         * For DEFAULT strategy: std::optional<FactoryCls>
         * For UNIQUE strategy: std::pair<ref_count, std::optional<FactoryCls>>
         */
        using vectorItem_t = std::conditional_t<
            Strategy == RegistrationStrategy::DEFAULT,
            std::optional<FactoryCls>,
            std::pair<std::size_t, std::optional<FactoryCls>>
            >;

        inline static std::size_t s_size = 0;                                           ///< Current active registration count
        inline static std::vector<vectorItem_t> s_collection;                           ///< Storage for all registrations
        static constexpr std::size_t INVALID_INDEX = ~0u;                               ///< Sentinel for invalid index
        std::size_t m_index = INVALID_INDEX;                                            ///< This object's index in s_collection

        /**
         * @brief Set the index of a Registrator object.
         *
         * @param registrator Registrator object to modify
         * @param index New index value
         */
        static void setIndex(Registrator& registrator, std::size_t index);

    public:
        /**
         * @brief Get the registration index for a given name.
         *
         * @param name Name to look up
         * @return Index in s_collection or INVALID_INDEX if not found
         */
        [[nodiscard]] static std::size_t getIndex(const std::string& name);

        /**
         * @brief Get the index of a Registrator object.
         *
         * @return The object's index (may be INVALID_INDEX)
         */
        [[nodiscard]] std::size_t getIndex() const;

    private:
        /**
         * @brief Private constructor used by Create() method.
         * @param index Index in s_collection for this registration
         */
        Registrator(std::size_t index);

    public:
        /**
         * @brief Create a new RAII registration.
         *
         * Creates a new registration entry and returns a Registrator object
         * that manages its lifetime. When the returned object is destroyed,
         * the registration is automatically cleaned up.
         *
         * @tparam T Type parameter passed to FactoryCls::create()
         * @param name Name for the registration
         * @return Registrator object managing the new registration
         *
         * @note For UNIQUE strategy: if name already exists, increments ref count
         * @note Use [[nodiscard]] to avoid accidental omission
         */
        template <typename T = void>
        [[nodiscard]] static Registrator Create(const std::string& name);

        /**
         * @brief Create a permanent registration without RAII management.
         *
         * Registers an entry that persists until explicitly removed or
         * until program termination. No RAII object is returned.
         *
         * @tparam T Type parameter passed to FactoryCls::create()
         * @param name Name for the registration
         *
         * @note For UNIQUE strategy: if name already exists, increments ref count
         * @warning Permanent registrations cannot be automatically cleaned up
         */
        template <typename T = void>
        static void Register(const std::string& name);

        /**
         * @brief Get a view of all active registrations.
         *
         * @return A range view containing all currently active FactoryCls objects
         *
         * @example
         * for (const auto& entry : Registrator<>::iter())
         * {
         *     std::cout << entry.name << std::endl;
         * }
         */
        [[nodiscard]] static auto iter() -> decltype(auto)
        {
            if constexpr (Strategy == RegistrationStrategy::DEFAULT)
            {
                return s_collection
                    | std::views::filter([](auto&& ptr) { return ptr.has_value(); })
                    | std::views::transform([](auto&& ptr) { return ptr.value(); });
            }
            else if constexpr (Strategy == RegistrationStrategy::UNIQUE)
            {
                return s_collection
                    | std::views::filter([](auto&& pairObj) { return pairObj.second.has_value(); })
                    | std::views::transform([](auto&& ptr) { return ptr.second.value(); });
            }
        }

        /**
         * @brief Get the number of active registrations.
         * @return Current count of active (non-null) registrations
         */
        [[nodiscard]] static std::size_t size();

        /**
         * @brief Look up a registration by name.
         * @param name Name to search for
         * @return Pointer to the FactoryCls object, or nullptr if not found
         */
        [[nodiscard]] static FactoryCls* get(const std::string& name);

        /**
         * @brief Look up a registration by index
         * @param key The index of the registration to retrieve
         * @return Pointer to the FactoryCls object, or nullptr if index is invalid or registration is inactive
         *
         * Provides direct index-based access to registrations. This is faster than
         * name-based lookup but requires knowing the index in advance.
         *
         * @pre key must be a valid index returned from getIndex()
         * @note Returns nullptr if the registration at that index has been destroyed
         *
         * @see getIndex()
         * @see get(const std::string&)
         */
        [[nodiscard]] static FactoryCls* get(std::size_t key);

        /**
         * @brief Checks whether a registration with the given name exists
         * @param name Name to check for
         * @return true if a registration with the specified name exists, false otherwise
         *
         * Performs a lookup to determine if a registration has been created with
         * the given name. This is useful for conditional registration logic.
         *
         * @note Complexity: O(n) where n is number of registrations
         * @see getIndex()
         * @see get()
         *
         * @par Example:
         * @code
         * if (!Registrator<>::contains("MyComponent")) {
         *     auto reg = Registrator<>::Create<MyComponent>("MyComponent");
         * }
         * @endcode
         */
        [[nodiscard]] static bool contains(const std::string& name);

    };

} // namespace reg

#include "detail/Registrator.ipp"