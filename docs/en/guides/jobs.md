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

## Threading rules

Backends must be safe to call from the main thread; worker-thread safety is
backend-defined. As elsewhere in R-ECS, **structural changes** (creating or
destroying entities, adding or removing components) must not happen from worker
threads — funnel them through the deferred [command queue](./commands.md), which
is flushed on the main thread. Reading and writing component values of disjoint
entities in parallel is safe.

## See also

- [Systems & scheduling](./systems-and-scheduling.md) — `ECS_ACCESS` already
  declares the read/write component sets the scheduler uses to derive data
  ordering; the same information is what drives parallel system execution.
- [Commands](./commands.md) — the main-thread sync point for structural changes.
