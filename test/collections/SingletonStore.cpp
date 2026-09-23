#include "collections/SingletonStore.hpp"
#include <gtest/gtest.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>


using namespace collections;

struct TestStruct
{
    int value = 42;
};

struct AnotherStruct
{
    std::string name;
    AnotherStruct(std::string n) : name(std::move(n)) {}
};

// Fixture для SingletonStore
class ContextTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        store = std::make_unique<SingletonStore<>>();
    }

    std::unique_ptr<SingletonStore<>> store;
};

TEST_F(ContextTest, EmplaceAndGet)
{
    auto& t = store->emplace<TestStruct>();
    EXPECT_EQ(t.value, 42);

    auto& t2 = store->get<TestStruct>();
    EXPECT_EQ(&t, &t2);
}

TEST_F(ContextTest, HasMethod)
{
    EXPECT_FALSE(store->has<TestStruct>());
    store->emplace<TestStruct>();
    EXPECT_TRUE(store->has<TestStruct>());
}

TEST_F(ContextTest, RemoveMethod)
{
    store->emplace<TestStruct>();
    EXPECT_TRUE(store->has<TestStruct>());

    store->remove<TestStruct>();
    EXPECT_FALSE(store->has<TestStruct>());
}


TEST_F(ContextTest, GetOrEmplaceReturnsExisting)
{
    auto& t1 = store->emplace<TestStruct>();
    t1.value = 123;

    auto& t2 = store->getOrEmplace<TestStruct>();
    EXPECT_EQ(t2.value, 123);
}


TEST_F(ContextTest, GetOrEmplaceCreatesNewIfMissing)
{
    EXPECT_FALSE(store->has<AnotherStruct>());

    auto& a = store->getOrEmplace<AnotherStruct>("Hello");
    EXPECT_EQ(a.name, "Hello");
    EXPECT_TRUE(store->has<AnotherStruct>());
}


TEST_F(ContextTest, MultipleTypesStoredSeparately)
{
    auto& t = store->emplace<TestStruct>();
    auto& a = store->emplace<AnotherStruct>("World");

    t.value = 555;
    a.name = "Test";

    auto& t2 = store->get<TestStruct>();
    auto& a2 = store->get<AnotherStruct>();

    EXPECT_EQ(t2.value, 555);
    EXPECT_EQ(a2.name, "Test");
}


// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
// Behavioral contract, exercised identically across every SingletonStore
// configuration: default (no Alloc/Mutex), a custom mutex only, a custom
// allocator only, and both together.
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

namespace
{
    struct TrackingAllocatorState
    {
        static inline int allocations = 0;
        static inline int deallocations = 0;

        static void reset()
        {
            allocations = 0;
            deallocations = 0;
        }
    };

    template<typename T>
    struct TrackingAllocator
    {
        using value_type = T;

        TrackingAllocator() noexcept = default;
        template<typename U>
        TrackingAllocator(const TrackingAllocator<U>&) noexcept {}

        T* allocate(std::size_t n)
        {
            ++TrackingAllocatorState::allocations;
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }

        void deallocate(T* p, std::size_t) noexcept
        {
            ++TrackingAllocatorState::deallocations;
            ::operator delete(p);
        }

        template<typename U>
        bool operator==(const TrackingAllocator<U>&) const noexcept { return true; }
    };

    struct DefaultConfig    { using Store = SingletonStore<>; };
    struct MutexOnlyConfig  { using Store = SingletonStore<void, std::mutex>; };
    struct AllocOnlyConfig  { using Store = SingletonStore<TrackingAllocator<int>>; };
    struct AllocMutexConfig { using Store = SingletonStore<TrackingAllocator<int>, std::mutex>; };
}

template<typename Config>
class SingletonStoreTypedTest : public ::testing::Test
{
protected:
    void SetUp() override { TrackingAllocatorState::reset(); }

    typename Config::Store store;
};

using SingletonStoreConfigs =
    ::testing::Types<DefaultConfig, MutexOnlyConfig, AllocOnlyConfig, AllocMutexConfig>;

class SingletonStoreConfigNames
{
public:
    template<typename T>
    static std::string GetName(int)
    {
        if constexpr (std::is_same_v<T, DefaultConfig>) return "Default";
        if constexpr (std::is_same_v<T, MutexOnlyConfig>) return "MutexOnly";
        if constexpr (std::is_same_v<T, AllocOnlyConfig>) return "AllocatorOnly";
        if constexpr (std::is_same_v<T, AllocMutexConfig>) return "AllocatorAndMutex";
    }
};

TYPED_TEST_SUITE(SingletonStoreTypedTest, SingletonStoreConfigs, SingletonStoreConfigNames);

TYPED_TEST(SingletonStoreTypedTest, EmplaceAndGetReturnSameInstance)
{
    auto& t = this->store.template emplace<TestStruct>();
    EXPECT_EQ(t.value, 42);
    EXPECT_EQ(&this->store.template get<TestStruct>(), &t);
}

TYPED_TEST(SingletonStoreTypedTest, HasReflectsPresence)
{
    EXPECT_FALSE(this->store.template has<TestStruct>());
    this->store.template emplace<TestStruct>();
    EXPECT_TRUE(this->store.template has<TestStruct>());
}

TYPED_TEST(SingletonStoreTypedTest, RemoveDropsInstance)
{
    this->store.template emplace<TestStruct>();
    ASSERT_TRUE(this->store.template has<TestStruct>());

    this->store.template remove<TestStruct>();
    EXPECT_FALSE(this->store.template has<TestStruct>());
}

TYPED_TEST(SingletonStoreTypedTest, RemoveOfMissingTypeIsNoop)
{
    EXPECT_NO_THROW(this->store.template remove<TestStruct>());
    EXPECT_FALSE(this->store.template has<TestStruct>());
}

TYPED_TEST(SingletonStoreTypedTest, GetOrEmplaceReturnsExistingInstance)
{
    auto& t1 = this->store.template emplace<TestStruct>();
    t1.value = 123;

    auto& t2 = this->store.template getOrEmplace<TestStruct>();
    EXPECT_EQ(t2.value, 123);
    EXPECT_EQ(&t1, &t2);
}

TYPED_TEST(SingletonStoreTypedTest, GetOrEmplaceCreatesNewIfMissing)
{
    EXPECT_FALSE(this->store.template has<AnotherStruct>());

    auto& a = this->store.template getOrEmplace<AnotherStruct>("Hello");
    EXPECT_EQ(a.name, "Hello");
    EXPECT_TRUE(this->store.template has<AnotherStruct>());
}

TYPED_TEST(SingletonStoreTypedTest, MultipleTypesStoredSeparately)
{
    auto& t = this->store.template emplace<TestStruct>();
    auto& a = this->store.template emplace<AnotherStruct>("World");

    t.value = 555;
    a.name = "Test";

    EXPECT_EQ(this->store.template get<TestStruct>().value, 555);
    EXPECT_EQ(this->store.template get<AnotherStruct>().name, "Test");
}

TYPED_TEST(SingletonStoreTypedTest, ReEmplacingSameTypeReplacesTheInstance)
{
    this->store.template emplace<TestStruct>().value = 111;
    auto& second = this->store.template emplace<TestStruct>();
    second.value = 222;

    // Only the surviving instance is touched here: the first one was destroyed
    // by the second emplace<T>() and must not be dereferenced (regression test
    // for a use-after-free that used to hide behind unordered_map::emplace()
    // silently discarding the "duplicate" insert).
    EXPECT_EQ(this->store.template get<TestStruct>().value, 222);
    EXPECT_EQ(&this->store.template get<TestStruct>(), &second);
}

TYPED_TEST(SingletonStoreTypedTest, ConstGetOnConstStore)
{
    this->store.template emplace<TestStruct>().value = 7;

    const auto& constStore = this->store;
    EXPECT_EQ(constStore.template get<TestStruct>().value, 7);
}

TYPED_TEST(SingletonStoreTypedTest, MoveConstructTransfersAllInstances)
{
    this->store.template emplace<TestStruct>().value = 77;
    this->store.template emplace<AnotherStruct>("moved");

    typename TypeParam::Store moved(std::move(this->store));

    EXPECT_EQ(moved.template get<TestStruct>().value, 77);
    EXPECT_EQ(moved.template get<AnotherStruct>().name, "moved");
}

TYPED_TEST(SingletonStoreTypedTest, MoveAssignReplacesDestinationContents)
{
    this->store.template emplace<TestStruct>().value = 88;

    typename TypeParam::Store other;
    other.template emplace<AnotherStruct>("placeholder");

    other = std::move(this->store);

    EXPECT_EQ(other.template get<TestStruct>().value, 88);
    EXPECT_FALSE(other.template has<AnotherStruct>());
}

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
// AllocatorPolicy: proves emplace/remove/re-emplace/destruction actually
// route through the configured allocator rather than plain new/delete, and
// that the default (Alloc = void) configuration leaves it untouched.
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

class SingletonStoreAllocatorTest : public ::testing::Test
{
protected:
    void SetUp() override { TrackingAllocatorState::reset(); }
};

TEST_F(SingletonStoreAllocatorTest, EmplaceAllocatesThroughCustomAllocator)
{
    SingletonStore<TrackingAllocator<int>> store;
    store.emplace<TestStruct>();

    EXPECT_EQ(TrackingAllocatorState::allocations, 1);
    EXPECT_EQ(TrackingAllocatorState::deallocations, 0);
}

TEST_F(SingletonStoreAllocatorTest, RemoveDeallocatesThroughCustomAllocator)
{
    SingletonStore<TrackingAllocator<int>> store;
    store.emplace<TestStruct>();

    store.remove<TestStruct>();
    EXPECT_EQ(TrackingAllocatorState::deallocations, 1);
}

TEST_F(SingletonStoreAllocatorTest, ReEmplaceDeallocatesThePreviousInstance)
{
    SingletonStore<TrackingAllocator<int>> store;
    store.emplace<TestStruct>();
    store.emplace<TestStruct>();

    EXPECT_EQ(TrackingAllocatorState::allocations, 2);
    EXPECT_EQ(TrackingAllocatorState::deallocations, 1);
}

TEST_F(SingletonStoreAllocatorTest, DestructorDeallocatesEveryRemainingInstance)
{
    {
        SingletonStore<TrackingAllocator<int>> store;
        store.emplace<TestStruct>();
        store.emplace<AnotherStruct>("x");

        EXPECT_EQ(TrackingAllocatorState::allocations, 2);
        EXPECT_EQ(TrackingAllocatorState::deallocations, 0);
    }

    EXPECT_EQ(TrackingAllocatorState::deallocations, 2);
}

TEST_F(SingletonStoreAllocatorTest, DefaultConfigurationDoesNotTouchCustomAllocator)
{
    SingletonStore<> store; // Alloc = void: plain new/delete
    store.emplace<TestStruct>();
    store.remove<TestStruct>();

    EXPECT_EQ(TrackingAllocatorState::allocations, 0);
    EXPECT_EQ(TrackingAllocatorState::deallocations, 0);
}

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
// MutexPolicy: proves the store is actually thread-safe when a real Mutex is
// configured, and that a real (non-movable) mutex type does not stop the
// store itself from being movable.
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

class SingletonStoreMutexTest : public ::testing::Test {};

TEST_F(SingletonStoreMutexTest, PolicyValueReflectsWhetherLockingIsEnabled)
{
    static_assert(!MutexPolicy<void>::value, "void Mutex must disable locking");
    static_assert(MutexPolicy<std::mutex>::value, "a real Mutex must enable locking");
}

TEST_F(SingletonStoreMutexTest, ConcurrentGetOrEmplaceConstructsExactlyOnce)
{
    static std::atomic<int> constructions{0};
    struct Counted { Counted() { ++constructions; } };
    constructions = 0;

    SingletonStore<void, std::mutex> store;
    constexpr int kThreads = 16;
    constexpr int kIterationsPerThread = 200;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i)
        threads.emplace_back([&store]
        {
            for (int j = 0; j < kIterationsPerThread; ++j)
                store.getOrEmplace<Counted>();
        });
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(constructions.load(), 1);
}

TEST_F(SingletonStoreMutexTest, ConcurrentReadOnlyAccessAfterMainThreadSetupIsSafe)
{
    // The documented safe pattern (see ecs::Context's own class docs): create the
    // shared instances up front on one thread, then let many threads read them
    // concurrently. Note this deliberately does NOT have multiple threads call
    // emplace<T>() on the *same* T concurrently and keep using the old reference:
    // emplace() is documented to invalidate any previous reference to T on every
    // call, by design (see emplace()'s @warning), so racing emplace<T>() itself
    // across threads is a misuse of the container, not something the mutex is
    // meant to make safe - it protects the map's internal bookkeeping, not the
    // lifetime of a reference returned by a competing replace.
    SingletonStore<void, std::mutex> store;
    store.emplace<TestStruct>().value = 555;
    store.emplace<AnotherStruct>("shared");

    constexpr int kThreads = 8;
    constexpr int kIterationsPerThread = 200;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i)
        threads.emplace_back([&store]
        {
            for (int j = 0; j < kIterationsPerThread; ++j)
            {
                EXPECT_EQ(store.get<TestStruct>().value, 555);
                EXPECT_TRUE(store.has<AnotherStruct>());
                // Already present, so this only ever reads - never replaces.
                EXPECT_EQ(store.getOrEmplace<AnotherStruct>("ignored").name, "shared");
            }
        });
    for (auto& t : threads)
        t.join();
}

TEST_F(SingletonStoreMutexTest, StoreStaysMovableWithANonMovableMutex)
{
    // Regression test: a naive `mutable Mutex m_mutex;` member makes the
    // implicitly-defaulted move ctor/assignment of MutexPolicy (and therefore
    // SingletonStore) deleted whenever Mutex itself is non-movable, which is
    // true of every standard mutex type (std::mutex, std::recursive_mutex, ...).
    static_assert(std::is_move_constructible_v<SingletonStore<void, std::mutex>>,
                  "SingletonStore must stay move-constructible even with std::mutex configured");
    static_assert(std::is_move_assignable_v<SingletonStore<void, std::mutex>>,
                  "SingletonStore must stay move-assignable even with std::mutex configured");

    SingletonStore<void, std::mutex> a;
    a.emplace<TestStruct>().value = 5;

    SingletonStore<void, std::mutex> b(std::move(a));
    EXPECT_EQ(b.get<TestStruct>().value, 5);

    SingletonStore<void, std::mutex> c;
    c = std::move(b);
    EXPECT_EQ(c.get<TestStruct>().value, 5);

    // The moved-to store must still be safely lockable/usable afterwards.
    c.emplace<AnotherStruct>("after-move");
    EXPECT_TRUE(c.has<AnotherStruct>());
}
