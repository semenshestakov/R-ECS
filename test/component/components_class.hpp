#pragma once
#include "ecs/component/Component.hpp"

using namespace ecs;
using namespace ecs::component;

enum ComponentsType : componentId_t
{
    POSITION = 1,
    POSITION_2D,
    POSITION_3D,

    DESTRUCTOR,
};


// inheritance
class Position2D_inheritance : public BaseComponent
{
public:
    static constexpr componentId_t componentId = ComponentsType::POSITION;
    float x, y;
    Position2D_inheritance(const float x, const float y) : x(x), y(y) {}
};


class Position3D_inheritance : public Position2D_inheritance
{
public:
    float z;
    Position3D_inheritance(const float x, const float y, const float z) :
        Position2D_inheritance(x, y), z(z)
    {}
};


// no_inheritance
class Position2D_no_inheritance : public BaseComponent
{
public:
    static constexpr componentId_t componentId = ComponentsType::POSITION_2D;
    float x {10}, y {12};
    Position2D_no_inheritance() = default;
    Position2D_no_inheritance(const float x, const float y) : x(x), y(y) {}
};


class Position3D_no_inheritance : public BaseComponent
{
public:
    static constexpr componentId_t componentId = ComponentsType::POSITION_3D;
    float x{}, y{}, z{};
    Position3D_no_inheritance() = default;
    Position3D_no_inheritance(const float x, const float y, const float z) :
        x(x), y(y), z(z)
    {}
};

