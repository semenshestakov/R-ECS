#pragma once
#include <memory>
#include <string_view>
#include "Utils.hpp"


namespace ecs
{

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
         * knowing their concrete type. This is essential for the SystemsManager
         * to create copies of registered system types.
         *
         * @return IBaseSystem* Pointer to a newly allocated copy of the system
         */
        [[nodiscard]] virtual IBaseSystem* New() const = 0;

        /**
         * @brief Returns the system name identifier.
         *
         * Used for debugging, logging, and system registration.
         *
         * @return std::string_view Name of the system.
         */
        [[nodiscard]] virtual std::string_view name() const { static constexpr char s_name[] = "IBaseSystem"; return s_name;};

        /**
         * @brief Initializes the system with provided configuration.
         *
         * Called once during system registration or application startup to
         * perform any necessary setup. Systems can override this method to
         * process initialization parameters from the InitState structure.
         * The default implementation does nothing.
         *
         * @throw DoubleInitialization if System is inited
         * @param state Structure containing initialization parameters
         */
        virtual void Init(const InitState& state);

        /**
         * @brief Subscribes the system to the event system.
         *
         * Called after initialization to register system callbacks into the
         * ECS event infrastructure. Derived systems must implement this method
         * to bind their event handlers using the provided EventSystem instance.
         *
         * @param state Structure containing event system reference and subscription parameters.
         */
        virtual void Subscribe(const SubscribeState& state) = 0;

        /**
         * @brief Performs the system's main logic for a single update cycle.
         *
         * Called once per frame or update tick to execute the system's behavior.
         * Systems receive the Registry to query and manipulate entities, along
         * with context about the current update phase.
         *
         * @param registry Reference to the main ECS Registry for entity operations
         * @param state Structure containing update context information
         */
        virtual void Update(Registry& registry, const UpdateState& state) {}

        /**
         * @brief Checks if the system has been initialized.
         * @return true if initialized, false otherwise.
         */
        [[nodiscard]] bool isInit() const { return m_isInit; }

        /**
         * @brief Returns the list of systems this system hard-depends on.
         * @return DependentSystems object containing system hashes.
         * @note Override this method (usually via ECS_DEPENDENT_SYSTEMS) to declare
         *       hard dependencies. Hard dependencies order execution and propagate
         *       disabling.
         */
        [[nodiscard]] virtual DependentSystems GetDependents() const { return {}; }

        /**
         * @brief Returns the list of systems this system weakly depends on.
         * @return DependentSystems object containing system hashes.
         * @note Override this method (usually via ECS_WEAK_DEPENDENT_SYSTEMS) to
         *       declare soft dependencies. Weak dependencies order execution but
         *       never propagate disabling.
         */
        [[nodiscard]] virtual DependentSystems GetWeakDependents() const { return {}; }

        /**
         * @brief Returns the component access this system declares.
         * @return ComponentAccess view describing the RO/WO/RW components used.
         * @note Override this method (usually via ECS_ACCESS) so the scheduler can
         *       derive data ordering edges (writer-before-reader) between systems.
         */
        [[nodiscard]] virtual ComponentAccess GetComponentAccess() const { return {}; }

    private:
        bool m_isInit = false;

    };

    inline void IBaseSystem::Init(const InitState& state)
    {
        if (m_isInit)
        {
            throw error::DoubleInitialization("Init::%s", this->name());
        }
        m_isInit = true;
    }
    using baseSystemPtr_t = std::unique_ptr<IBaseSystem>;                           ///< Type alias for system ownership

} // namespace ecs
