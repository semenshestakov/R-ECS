#pragma once
#include <cstdint>
#include <optional>


namespace ecs
{
    using entityId_t = std::uint64_t;

    using entityOpt_t = std::optional<entityId_t>;
    constexpr entityOpt_t entityNull = std::nullopt;

    class Components;

    struct Entity
    {
        entityId_t id;
        Components& components;
        [[nodiscard]] Components& operator->() const { return components; }
        [[nodiscard]] Components& operator* () const { return components; }
        auto operator<=>(const Entity &) const { return id; }
    };
}
