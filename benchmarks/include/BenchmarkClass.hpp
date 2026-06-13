#pragma once


struct Position2d final
{
    float x {}, y {};
};

struct Position3d final
{
    float x {1}, y {2}, z{3};
};

struct Velocity2d final
{
    float vx {1.f}, vy {0.f};
};

struct Health final
{
    float hp {100.f};
};

struct UnuseStruct final
{
    int v = 0;
};