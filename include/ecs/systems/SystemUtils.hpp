#pragma once


namespace ecs
{
    class Registry;

    using updateTag_t = unsigned short;
    constexpr updateTag_t MAX_UPDATE_TAG = ~0;

    struct InitState
    {
        const char* nameFactory;
        void* args;
    };

    struct UpdateState
    {
        updateTag_t updateTag;
    };

    struct IBaseSystem
    {
        IBaseSystem() = default;
        virtual ~IBaseSystem() = default;
        [[nodiscard]] virtual IBaseSystem* New() const = 0;
        virtual void Init(const InitState& state) {}
        virtual void Update(Registry& registry, const UpdateState& state) = 0;
        static constexpr updateTag_t UPDATE_TAG = MAX_UPDATE_TAG / 2;
    };

}
