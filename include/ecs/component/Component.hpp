#pragma once


namespace ecs::component
{

    using byte = unsigned char;
    using bufferSize_t = unsigned int;
    using componentId_t = byte;

    constexpr componentId_t INVALID_COMPONENT_ID = 0;

    class BaseComponent
    {
    public:
        static constexpr componentId_t componentId = INVALID_COMPONENT_ID;
        virtual ~BaseComponent() = default;
    };

}
