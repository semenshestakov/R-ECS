# Jobs & threading

[🇷🇺 Русский](../../ru/guides/jobs.md) · [⬆ English docs](../README.md)

R-ECS does not ship a thread pool and is **not** tied to any task library. The
core depends on a single abstract port — `ecs::IJobScheduler` — and a concrete
backend is injected at runtime. This lets you plug in TBB, an `std::thread`
pool, or your own scheduler without touching ECS or system code.

## The port

[`ecs/jobs/IJobScheduler.hpp`](../../../include/ecs/jobs/IJobScheduler.hpp)
declares the only multithreading dependency the core has. No header under
`include/ecs` includes a threading library.

```cpp
class IJobScheduler
{
public:
    using RangeBody = FunctionRef<void(std::size_t first, std::size_t last)>;

    virtual void      ParallelFor(std::size_t begin, std::size_t end,
                                  std::size_t grain, RangeBody body) = 0;
    virtual JobHandle Run(std::function<void()> job)                 = 0;
    virtual void      Wait(const JobHandle& handle)                  = 0;
    virtual std::size_t WorkerCount() const                          = 0;
};
```

The interface is **coarse by design**: every method is a per-system / per-frame
entry point, so the one virtual dispatch per call is negligible against the work
it launches. The per-element work travels in the `RangeBody`, a
[`FunctionRef`](../../../include/ecs/jobs/FunctionRef.hpp) — a non-owning,
allocation-free callable reference — and runs without further indirection.

`Run` returns a [`JobHandle`](../../../include/ecs/jobs/JobHandle.hpp): an opaque
value type. The core never inspects it; the backend stores whatever it needs
(a task group, a future, a latch) behind type-erased state and casts it back in
its own `Wait`.

## The default backend

Every `Registry` always has a valid, non-null scheduler. With nothing installed
it uses [`SerialJobScheduler`](../../../include/ecs/jobs/SerialJobScheduler.hpp),
which runs all work inline on the calling thread. The core is therefore fully
functional with no backend attached, and single-threaded builds pay nothing.

## Installing a backend

Inject a real pool with `Registry::SetScheduler`. No system code changes.

```cpp
auto registry = ecs::Registry::Create("game");
registry.SetScheduler(std::make_unique<MyTbbScheduler>(/*threads*/ 8));
registry.Init();
```

Passing `nullptr` resets to the serial backend, so the registry never holds a
null scheduler.

## Writing a backend

Implement the four methods. A `Run` schedules a task and returns a handle whose
state is signalled on completion; `Wait` joins it. `ParallelFor` splits
`[begin, end)` into chunks no smaller than `grain` and applies `body` to each,
possibly concurrently, blocking until all chunks finish. `body` must be safe to
run concurrently on disjoint sub-ranges.

```cpp
class MyTbbScheduler final : public ecs::IJobScheduler
{
    void ParallelFor(std::size_t begin, std::size_t end,
                     std::size_t grain, RangeBody body) override
    {
        tbb::parallel_for(
            tbb::blocked_range<std::size_t>(begin, end, grain),
            [&](const tbb::blocked_range<std::size_t>& r) { body(r.begin(), r.end()); });
    }
    // Run / Wait / WorkerCount ...
};
```

The backend lives in its **own** target and links TBB; the ECS core is built and
tested without it. This is plain dependency inversion: the core depends on the
port, the backend depends on both the core and its threading library.

## Parallel system execution

`SystemsManager::Update` dispatches each schedule **stage** through
`Registry::Scheduler().ParallelFor`. A stage is a set of systems with no
ordering edges between them — exactly the systems that are safe to run at the
same time — so their `Update` calls are handed to the scheduler and may run on
several workers at once. Because `ParallelFor` blocks until the stage finishes,
the end of every stage is an implicit barrier, and stage *N+1* (which may depend
on stage *N*) only starts once stage *N* has fully completed. With the default
`SerialJobScheduler` everything still runs inline, so behaviour is unchanged
until you install a real backend.

This is the per-frame ("frame") work. Long-lived, non-frame jobs a backend may
own (dedicated network/render threads, background streaming) live outside the
update loop; because the `Registry` owns the scheduler, those jobs are torn down
in the backend's destructor when the `Registry` dies.

## Threading rules

Backends must be safe to call from the main thread; worker-thread safety is
backend-defined. As elsewhere in R-ECS, **structural changes** (creating or
destroying entities, adding or removing components, registering systems) must
not happen from worker threads — funnel them through the deferred
[command queue](./commands.md), which is flushed on the main thread. Reading and
writing component values of disjoint entities in parallel is safe.

These rules are enforced in debug builds. The application designates its main
thread by calling `ecs::MarkMainThread()` once at startup (the owning of this
policy is the application's, not the `Registry`'s) — for example at the top of
`main`, before creating the registry and running the frame loop. The structural
/ single-threaded entry points then assert via `ECS_ASSERT_MAIN_THREAD` that they
are called from that thread:

- entities — `EntitiesManager::Create` / `Destroy` / `AddComponents` / `RemoveComponents`;
- systems — `SystemsManager::Register` / `Enable` / `Disable`;
- events — `EventSystem::OnEvent` / `PushEvent` / `FlushEvents`;
- context — `Context::emplace` / `getOrEmplace` / `remove`.

Two adjacent operations are instead made **thread-safe by design**, so they
carry no main-thread restriction:

- **Command queue.** `CommandQueue::Push` is the deferral channel for systems
  and is callable from any thread: concurrent pushes are serialized by an
  internal mutex. A parallel system queues its structural changes here, and they
  run on the Registry's thread at the next `Flush()`, which takes that same mutex
  only to swap the pending commands out, then executes them without the lock (so
  it can't race a concurrent `Push`, never holds the lock across user code, and a
  command may itself `Push` without deadlock).
- **Component registration.** `ComponentRegistrator::Register` runs lazily the
  first time `GetComponentId<T>()` is reached — which can be deep inside a
  `view<...>` iteration. The per-type magic static bounds it to once per type,
  and a mutex serializes two *different* types registering concurrently, so it
  is safe even when first touched in parallel. Registration is rare; warming up
  components on the main thread during setup simply avoids paying the lock at
  all on the hot path.

Calling a guarded operation from a worker — e.g. directly mutating entities
inside a parallel system instead of queuing a command — trips the assert. Until
`ecs::MarkMainThread()` runs, the checks stay inactive (they report success), so
forgetting to call it never produces a false positive — only a missed check. The
check compiles to nothing under `NDEBUG`, so release builds pay nothing.

R-ECS's own test and benchmark runners call `ecs::MarkMainThread()` at the very
start of `main`, so the affinity checks are live throughout the suite.

`ecs::Context` is a thin ECS-layer adapter over the generic
`collections::Context` (the same way `ecs::EventSystem` wraps the generic event
dispatcher): it guards only the mutating operations, while read-only `get` /
`has` are inherited unchanged and remain safe to call concurrently from workers
on entries that already exist. Create shared resources on the main thread, then
read them with `get<T>()` from parallel systems.

## See also

- [Systems & scheduling](./systems-and-scheduling.md) — `ECS_ACCESS` already
  declares the read/write component sets the scheduler uses to derive data
  ordering; the same information is what drives parallel system execution.
- [Commands](./commands.md) — the main-thread sync point for structural changes.
