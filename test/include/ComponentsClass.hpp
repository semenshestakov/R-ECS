#pragma once
#include "ecs/entities/EntityWrapper.hpp"
#include "ecs/entities/PrefabEntity.hpp"


struct Position2d final
{
    float x {}, y {};
};

struct Position3d final
{
    float x {1}, y {2}, z{3};
    void* testLeaks = nullptr;

    Position3d() : Position3d(1, 2, 3) {}
    Position3d(const float x) : Position3d(x, 2.f, 3.f) {}
    Position3d(const float x, const float y) : Position3d(x, y, 3.f) {}
    Position3d(const float x, const float y, const float z) : x(x), y(y), z(z)
    {
        testLeaks = malloc(sizeof(Position3d));
    }

    Position3d(const Position3d& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
    }

    Position3d(Position3d&& other) noexcept
    {
        std::swap(testLeaks, other.testLeaks);
        std::swap(x, other.x);
        std::swap(y, other.y);
        std::swap(z, other.z);
    }

    Position3d& operator=(const Position3d& other)
    {
        testLeaks = malloc(sizeof(Position3d));
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    Position3d& operator=(Position3d&& other) noexcept
    {
        std::swap(testLeaks, other.testLeaks);
        std::swap(x, other.x);
        std::swap(y, other.y);
        std::swap(z, other.z);
        return *this;
    }

    ~Position3d()
    {
        free(testLeaks);
    }
};

struct DestructorTest final
{
    inline static bool testValue = false;
    ~DestructorTest() {testValue = !testValue;}
};

struct PositionX
{
    float x {1};
};

struct PositionY
{
    float y {2};
};

struct PositionZ
{
    float z {3};
};

struct TestId
{
    unsigned int id {0};
};

struct Health
{
    float value = 100.f;
};

struct Damage
{
    float value = 10.f;
};

struct Speed
{
    float value = 1.f;
};

struct Player final : ecs::EntityWrapper
{
    using EntityWrapper::EntityWrapper;

    [[nodiscard]] float GetHealth() const { return GetComponent<Health>().value; }
    void SetHealth(const float v) { GetComponent<Health>().value = v; }
    [[nodiscard]] Position2d& GetPosition() { return GetComponent<Position2d>(); }
    [[nodiscard]] const Position2d& GetPosition() const { return GetComponent<Position2d>(); }
};

struct Enemy final : ecs::EntityWrapper
{
    using EntityWrapper::EntityWrapper;

    [[nodiscard]] float GetHealth() const { return GetComponent<Health>().value; }
    [[nodiscard]] float GetDamage() const { return GetComponent<Damage>().value; }
    [[nodiscard]] Position2d& GetPosition() { return GetComponent<Position2d>(); }
    [[nodiscard]] const Position2d& GetPosition() const { return GetComponent<Position2d>(); }
};

struct Projectile final : ecs::EntityWrapper
{
    using EntityWrapper::EntityWrapper;

    [[nodiscard]] Position3d& GetPosition() { return GetComponent<Position3d>(); }
    [[nodiscard]] const Position3d& GetPosition() const { return GetComponent<Position3d>(); }
    [[nodiscard]] float GetSpeed() const { return GetComponent<Speed>().value; }
};

struct PlayerRecipe
{
    static void apply(ecs::PrefabEntity& p)
    {
        p.AddComponent<Health>(100.f);
        p.AddComponent<Position2d>(0.f, 0.f);
    }
};

struct EnemyRecipe
{
    static void apply(ecs::PrefabEntity& p)
    {
        p.AddComponent<Health>(50.f);
        p.AddComponent<Damage>(15.f);
        p.AddComponent<Position2d>(10.f, 10.f);
    }
};

struct ProjectileRecipe
{
    static void apply(ecs::PrefabEntity& p)
    {
        p.AddComponent<Position3d>(1.f, 2.f, 3.f);
        p.AddComponent<Speed>(100.f);
    }
};

struct FullscreenConfig
{
    bool enabled = true;
};

struct Camera final : ecs::EntityWrapper
{
    using EntityWrapper::EntityWrapper;

    [[nodiscard]] Position3d& GetPosition() { return GetComponent<Position3d>(); }
    [[nodiscard]] const Position3d& GetPosition() const { return GetComponent<Position3d>(); }
    [[nodiscard]] bool IsFullscreen() const { return GetComponent<FullscreenConfig>().enabled; }
};

struct CameraRecipe
{
    static void apply(ecs::PrefabEntity& p)
    {
        p.AddComponent<Position3d>(0.f, 0.f, 0.f);
        p.AddComponent<FullscreenConfig>();
    }
};

template<std::size_t N>
struct TestComponent
{
    std::size_t n = N;
};


template<size_t... Is>
void ValidateEntity(const ecs::EntityWrapper& entity, std::index_sequence<Is...>)
{
    auto checker = [&]<size_t I>()
    {
    EXPECT_EQ(entity.GetComponent<TestComponent<I>>().n, I);
    };

    (checker.template operator()<Is>(), ...);
}

template<size_t Count, size_t... Is>
void BuildPrefab(ecs::PrefabEntity& prefab, std::index_sequence<Is...>)
{
    ((Is < Count
        ? (void)prefab.AddComponent<TestComponent<Is>>()
        : (void)0), ...);
}

template<size_t ArchetypeSize>
ecs::PrefabEntity CreateArchetype()
{
    ecs::PrefabEntity prefab;

    [&]<size_t... Is>(std::index_sequence<Is...>)
    {
    ((Is < ArchetypeSize
        ? (void)prefab.AddComponent<TestComponent<Is>>()
        : (void)0), ...);
    }
    (std::make_index_sequence<1000>{});

    return prefab;
}


template<size_t Id, size_t Begin, size_t End>
void ValidateOne(const ecs::EntityWrapper& entity)
{
    auto* component =
        entity.TryGetComponent<TestComponent<Id>>();

    if constexpr (Id >= Begin && Id < End)
    {
        ASSERT_NE(component, nullptr);
        EXPECT_EQ(component->n, Id);
    }
    else
    {
        EXPECT_EQ(component, nullptr);
    }
}


template<size_t Begin, size_t End, size_t... Is>
void ValidateRange(const ecs::EntityWrapper& entity, std::index_sequence<Is...>)
{
    (ValidateOne<Is, Begin, End>(entity), ...);
}


struct MoveTracker
{
    inline static int copyCount = 0;
    inline static int moveCount = 0;

    int value = 0;

    explicit MoveTracker(const int v = 0) : value(v) {}
    MoveTracker(const MoveTracker& other) : value(other.value)          { ++copyCount; }
    MoveTracker(MoveTracker&& other) noexcept : value(other.value)      { other.value = -1; ++moveCount; }

    static void reset() { copyCount = moveCount = 0; }
};
