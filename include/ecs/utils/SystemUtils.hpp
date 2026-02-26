#pragma once


namespace ecs
{
    class Registry;

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
