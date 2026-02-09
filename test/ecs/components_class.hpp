#pragma once
#include "ecs/IComponent.hpp"


struct Position2d final : ecs::IComponent<Position2d>
{
    Position2d() = default;
    Position2d(const float x, const float y) : x(x), y(y) {};
    float x, y;
};


struct Position3d final : ecs::IComponent<Position3d>
{
    float x {1}, y {2}, z{3};
};


struct DestructorTest final : ecs::IComponent<DestructorTest>
{
    inline static bool testValue = false;
    ~DestructorTest() {testValue = !testValue;}
};


struct PositionX : ecs::IComponent<PositionX>
{
    ECS_REGISTRY("test1", "test2")
    float x {1};
};

struct PositionY : ecs::IComponent<PositionY>
{
    ECS_REGISTRY("test1")
    float y {2};
};

struct PositionZ : ecs::IComponent<PositionZ>
{
    float z {3};
};

struct TestId : ecs::IComponent<TestId>
{
    ECS_REGISTRY("test1")
    unsigned int id {0};
};
