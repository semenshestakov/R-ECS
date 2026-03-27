#pragma once
#include "../ComponentRegistrator.hpp"


namespace ecs
{

    template<typename ComponentCls>
    RegisterComponentInfo RegisterComponentInfo::Create(const std::string& name)
    {
        return {
            .name=std::string(name),
            .componentSize=sizeof(ComponentCls),
            .componentId=ComponentCls::componentId,
            .constructor=[](byte* ptr) { new (ptr) ComponentCls(); },
            .destructor=[](byte* ptr) { reinterpret_cast<ComponentCls*>(ptr)->~ComponentCls(); }
        };
    }

    template<typename ComponentCls>
    componentId_t ComponentRegistrator::Register(const std::string& name)
    {
        Super registrator = Super::Create<ComponentCls>(name);

        const std::size_t& index = Super::getIndex(registrator);
        Super::setIndex(registrator, Super::INVALID_INDEX);

        const auto componentId = static_cast<componentId_t>(index + 1);
        Super::s_collection[index]->componentId = componentId;
        return componentId;
    }

    /* static */ inline const RegisterComponentInfo& ComponentRegistrator::GetInfo(const componentId_t componentId)
    {
        return Super::s_collection[static_cast<std::size_t>(componentId - 1)].value();
    }

} // namespace ecs
