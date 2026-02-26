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
        const char* nameFactory;            ///< Identifier for the factory or creator of this system
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


    /**
     * @brief Abstract base interface that all systems must implement.
     *
     * Defines the contract for systems in the ECS architecture. Systems encapsulate
     * game logic that operates on entities with specific component combinations.
     * This interface provides lifecycle methods for initialization and per-frame
     * updates, along with a cloning mechanism for polymorphic system creation.
     * Each system can optionally specify an update tag to control its execution
     * phase in the update loop.
     */
    struct IBaseSystem
    {
        /**
         * @brief Default constructor for base system.
         *
         * Initializes the base system with default values. Derived systems
         * should perform their specific initialization in their constructors
         * or in the Init method.
         */
        IBaseSystem() = default;

        /**
         * @brief Virtual destructor ensuring proper cleanup of derived systems.
         *
         * Enables polymorphic destruction, allowing systems to be deleted through
         * base class pointers while ensuring derived class destructors are called.
         */
        virtual ~IBaseSystem() = default;

        /**
        * @brief Creates a new instance of the same system type.
         *
         * Virtual constructor pattern that enables cloning of systems without
         * knowing their concrete type. This is essential for the SystemManager
         * to create copies of registered system types.
         *
         * @return IBaseSystem* Pointer to a newly allocated copy of the system
         */
        [[nodiscard]] virtual IBaseSystem* New() const = 0;

        /**
         * @brief Initializes the system with provided configuration.
         *
         * Called once during system registration or application startup to
         * perform any necessary setup. Systems can override this method to
         * process initialization parameters from the InitState structure.
         * The default implementation does nothing.
         *
         * @param state Structure containing initialization parameters
         */
        virtual void Init(const InitState& state) {}

        /**
         * @brief Performs the system's main logic for a single update cycle.
         *
         * Called once per frame or update tick to execute the system's behavior.
         * Systems receive the Registry to query and manipulate entities, along
         * with context about the current update phase. This pure virtual method
         * must be implemented by all concrete system types.
         *
         * @param registry Reference to the main ECS Registry for entity operations
         * @param state Structure containing update context information
         */
        virtual void Update(Registry& registry, const UpdateState& state) = 0;

        /**
         * @brief Default update tag for systems that don't specify their own.
         *
         * Systems can override this constant to specify which update phase they
         * belong to. The default value is the midpoint of the possible tag range,
         * providing a balanced default position in the update order. Systems that
         * don't override this will be grouped together with this default tag.
         */
        static constexpr updateTag_t UPDATE_TAG = MAX_UPDATE_TAG / 2;
    };

}
