#pragma once


struct Position2d final
{
    float x {}, y {};
};

struct Position3d final
{
    float x {1}, y {2}, z{3};
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
