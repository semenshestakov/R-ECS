#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "collections/DirectedAcyclicGraph.hpp"
#include "IBaseSystem.hpp"


namespace ecs
{

    /**
     * @brief Manages the execution schedule of systems based on their dependencies.
     *
     * The SystemsSchedule class is responsible for constructing and managing the
     * execution order of systems in a dependency-driven architecture. It uses a
     * directed acyclic graph (DAG) to model dependencies between systems and
     * computes an optimal execution schedule where independent systems can run
     * in parallel.
     *
     * This class is a core component of systems-based architectures (like ECS or
     * game engines) where systems need to execute in a specific order based on
     * their dependencies. It automatically resolves dependency chains and
     * organizes systems into parallel execution stages.
     *
     * Key features:
     * - Automatic dependency resolution using topological sorting
     * - Parallel execution stage calculation for optimal performance
     * - Lazy initialization: schedule is computed only when Init() is called
     * - Type-safe system registration using hash-based identification
     * - STL-compatible iteration over execution stages
     *
     * The schedule construction process:
     * 1. Systems are added with their dependencies via Add() method
     * 2. Graph structure is built incrementally during Add() calls
     * 3. Init() performs topological sort to create execution stages
     * 4. Resulting stages can be iterated to execute systems in correct order
     *
     * @note The class is designed for single-threaded initialization. Once Init()
     *       is called, no new systems can be added without calling clear() first.
     *
     * @warning The schedule is invalidated after any modification. Call Init()
     *          again to regenerate the execution stages.
     *
     * @example
     * @code
     * // Create schedule manager
     * SystemsSchedule schedule;
     *
     * // Add systems with dependencies
     * auto renderHash = getSystemHash<RenderSystem>();
     * auto physicsHash = getSystemHash<PhysicsSystem>();
     * auto inputHash = getSystemHash<InputSystem>();
     *
     * schedule.Add(renderSystem, renderHash);
     * schedule.Add(physicsSystem, physicsHash);
     * schedule.Add(inputSystem, inputHash);
     *
     * // Initialize the schedule (computes topological order)
     * schedule.Init();
     *
     * // Execute systems in stages
     * for (const auto& stage : schedule) {
     *     for (auto systemHash : stage) {
     *         // Execute all systems in this stage in parallel
     *         executeSystem(systemHash);
     *     }
     * }
     * @endcode
     */
    class SystemsSchedule final
    {
        /**
         * @brief Type alias for the internal dependency graph.
         *
         * Uses DirectedAcyclicGraph with system hashes as nodes. Each edge stores
         * systemDepFlags_t (Direct and/or Data) describing how the dependency was
         * established.
         */
        using systemsGraph_t = collections::DirectedAcyclicGraph<systemHash_t, systemDepFlags_t>;

    public:
        /**
         * @brief Default constructor.
         *
         * Creates an empty schedule with no systems. The schedule is uninitialized
         * and must be populated with Add() calls before calling Init().
         */
        SystemsSchedule();

        /**
         * @brief Destructor.
         *
         * Cleans up all internal resources. All stored system references and
         * dependency information are properly destroyed.
         */
        ~SystemsSchedule();

        /**
         * @brief Adds a system and its dependencies to the schedule.
         *
         * Registers a system with the scheduler along with its hash identifier.
         * The system's dependencies are automatically extracted via GetDependents()
         * and added to the dependency graph.
         *
         * This method can be called multiple times to add different systems.
         * Each call accumulates systems and their dependencies. The method is
         * idempotent for the same system hash - subsequent calls update the
         * system reference but don't duplicate the system in the graph.
         *
         * @param system Reference to the system being added.
         * @param hash Unique hash identifier for the system.
         *
         * @note This method only records the system and marks the schedule dirty;
         *       it does not rebuild the execution stages. The topological sort and
         *       data-edge derivation are deferred to the next Build() call (which
         *       SystemsManager triggers from Update()/Subscribe()).
         *
         * @warning The system must remain valid for the lifetime of the schedule
         *          if any dependencies are needed. The schedule stores only the
         *          hash, not the system reference.
         */
        void Add(const IBaseSystem& system, systemHash_t hash);

        /**
         * @brief Initializes the execution schedule.
         *
         * Performs topological sorting of all added systems to compute the
         * execution stages. This method:
         * 1. Builds the complete dependency graph from all added systems
         * 2. Detects cycles in the dependency graph
         * 3. Computes topological order using Kahn's algorithm
         * 4. Groups systems into parallel execution stages
         *
         * The schedule is considered initialized after this call. Subsequent
         * modifications (Add()) will invalidate the schedule and require
         * another Init() call.
         *
         * @throws std::runtime_error if a cycle is detected in the dependency graph.
         *         The error message indicates the cycle exists but not which nodes.
         *
         * @note This method must be called at least once before iterating over
         *       the schedule stages. It can be called multiple times to regenerate
         *       the schedule after modifications.
         *
         * @warning The schedule becomes read-only after Init() until clear()
         *          is called or more systems are added.
         *
         */
        void Init();

        /**
         * @brief Rebuilds the execution stages only if the schedule is dirty.
         *
         * Lazy counterpart to Init(): if any system was added (or the schedule was
         * cleared) since the last build, this derives data edges and re-runs the
         * topological sort; otherwise it is a cheap no-op. SystemsManager calls it
         * at the start of Update() and Subscribe(), so dependency recomputation
         * happens at update time rather than during Add().
         *
         * @throws std::runtime_error if a cycle is detected in the dependency graph.
         */
        void Build();

        /**
         * @brief Reports whether the schedule needs rebuilding.
         * @return true if a system was added or the schedule cleared since the last build.
         */
        [[nodiscard]] bool isDirty() const { return m_dirty; }

        /**
        * @brief Clears all systems and resets the schedule.
        *
        * Removes all systems from the schedule and resets the initialization state.
        * After calling clear(), the schedule is empty and uninitialized, ready to
        * accept new systems via Add().
        *
        * This method is useful for:
        * - Rebuilding the schedule with a different set of systems
        * - Cleaning up resources when the schedule is no longer needed
        * - Resetting to a clean state for testing
        *
        * @note This invalidates any previously computed schedule stages.
        *       Iterators obtained before calling clear() become invalid.
        */
        void clear();

        /**
          * @brief Returns an iterator to the beginning of the execution stages.
          *
          * Provides access to the computed execution stages after Init() has been called.
          * Each element in the iteration is a vector of system hashes that can be
          * executed in parallel.
          *
          * @return Iterator to the first execution stage.
          * @pre Init() must have been called successfully before using this iterator.
          *
          * @warning The behavior is undefined if Init() hasn't been called or if the
          *          schedule has been modified after the last Init() call.
        */
        [[nodiscard]] auto begin() const {return m_stagesGraph.begin();}

        /**
         * @brief Returns an iterator to the end of the execution stages.
         *
         * @return Iterator to one past the last execution stage.
         * @pre Init() must have been called successfully.
         */
        [[nodiscard]] auto end() const {return m_stagesGraph.end();}

        /**
         * @brief Collects every system that transitively hard-depends on a root.
         *
         * Walks the dependency graph backwards along Direct (hard) edges only,
         * starting from each system in @p roots, and returns all systems reachable
         * that way. Data edges are ignored, so systems coupled only through shared
         * component access (or ECS_WEAK_DEPENDENT_SYSTEMS) are never collected.
         *
         * This is the primitive used to cascade disabling: disabling the roots also
         * disables everything returned here.
         *
         * @param roots The systems whose hard dependents should be collected.
         * @return The set of transitive hard dependents (excluding the roots themselves
         *         unless a root also depends on another root).
         */
        [[nodiscard]] std::unordered_set<systemHash_t> CollectHardDependents(const std::unordered_set<systemHash_t>& roots) const;

    private:
        /**
         * @brief Adds (or merges) a dependency edge, OR-ing the given flags in.
         * @param node The dependent system.
         * @param dep The system that must run before @p node.
         * @param flags The dependency flags to add to the edge.
         */
        void addEdge(systemHash_t node, systemHash_t dep, systemDepFlags_t flags);

        /**
         * @brief Derives Data ordering edges from declared component access.
         *
         * For every component touched by more than one system: orders readers after
         * writers, and orders two writers deterministically by system hash. Called
         * from Init() before the topological sort.
         */
        void deriveDataEdges();

        systemsGraph_t m_graph;                         ///< Internal dependency graph of all systems
        systemsGraph_t::stagesGraph_t m_stagesGraph;    ///< Computed execution stages after Init
        std::unordered_map<systemHash_t, std::vector<ComponentAccessEntry>> m_access; ///< Declared component access per system
        bool m_isInit = false;                          ///< Flag indicating if schedule has been built at least once
        bool m_dirty = false;                           ///< Set by Add()/clear(); cleared by Build()/Init()
    };

} // namespace ecs
