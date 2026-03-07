#pragma once


namespace event
{
    /**
     * @brief Forward declaration of the EventSystem template class
     * @tparam K The key type used to identify events
     */
    template<typename K /* key */> class EventSystem;
}


namespace ecs
{
    // Forward declarations
    class Registry;


    /**
     * @brief Type used as key for event identification in the ECS
     *
     * Events are identified by unique keys derived from their types.
     * Using std::size_t provides efficient hashing and lookup in containers.
     */
    using eventKey_t = std::size_t;

    /**
     * @brief Generates a unique compile-time key for an event type
     *
     * @tparam Event The event type to generate a key for
     * @return constexpr std::size_t A unique hash code for the event type
     *
     * @note Uses typeid(Event).hash_code() which is constexpr-friendly
     *       and provides a unique identifier per type at compile time
     *
     * @par Example:
     * @code
     * struct PlayerDiedEvent {};
     * auto key = getEventKey<PlayerDiedEvent>(); // Unique identifier
     * @endcode
     */
    template<class Event>
    constexpr std::size_t getEventKey()
    {
        return typeid(Event).hash_code();
    }

    /**
     * @brief ECS-specific event system type alias
     *
     * Specializes the generic EventSystem to use eventKey_t (std::size_t)
     * as the key type for event identification within the ECS.
     */
    using EventSystem = event::EventSystem<eventKey_t>;

    /**
     * @brief Type alias for update tags used to categorize system update phases.
     *
     * Update tags allow systems to be grouped into distinct update phases,
     * enabling controlled execution order and separation of concerns. Common
     * tags might represent phases like "PreUpdate", "Update", "PostUpdate",
     * or "Render". The unsigned short type provides 65535 possible tag values.
     */
    using updateTag_t = unsigned short;

    /**
     * @brief Maximum possible value for an update tag, used as a sentinel or default.
     *
     * This constant represents the highest value an updateTag_t can hold (~0
     * evaluates to 65535 for unsigned short). It can be used as a special marker
     * for default tags or to indicate uninitialized/end-of-range conditions.
     */
    constexpr updateTag_t MAX_UPDATE_TAG = ~0;

    /**
     * @brief Structure containing initialization parameters for system startup.
     *
     * Provides a flexible container for passing configuration data to systems
     * during their initialization phase. The structure uses a void pointer for
     * arguments to support any configuration type, making it extensible without
     * modifying the base interface.
     */
    struct InitState
    {
        const char* nameFactory;            ///< Identifier for the componentsManager or creator of this system
        void* args;                         ///< Pointer to system-specific initialization arguments
        EventSystem& eventSystem;           ///< Ref local Event System for ECS
    };

    /**
     * @brief Structure containing context information for system updates.
     *
     * Passed to systems during each update cycle, providing contextual information
     * about the current update phase. Currently contains the update tag, which
     * allows systems to know which phase they're being executed in and potentially
     * adjust their behavior accordingly.
     */
    struct UpdateState
    {
        updateTag_t updateTag;          ///< The tag identifying the current update phase
    };

}
