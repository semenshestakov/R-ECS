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
