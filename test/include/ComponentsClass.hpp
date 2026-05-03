#pragma once


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
