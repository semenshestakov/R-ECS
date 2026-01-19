#pragma once
#include "../ComponentRegistrator.hpp"


namespace ecs
{

    template<typename ComponentCls>
    RegisterComponentInfo RegisterComponentInfo::create(const std::string &name)
    {
        return {
            .name=name,
            .componentSize=sizeof(ComponentCls),
            .componentId=ComponentCls::componentId,
            .constructor=[](byte* ptr) { new (ptr) ComponentCls(); }
        };
    }

    template<typename ComponentCls>
    componentId_t ComponentRegistrator::Register(const std::string &name)
    {
        Super registrator = Super::Create<ComponentCls>(name);

        const std::size_t& index = Super::getIndex(registrator);
        Super::setIndex(registrator, Super::INVALID_INDEX);

        const auto componentId = static_cast<componentId_t>(index + 1);
        Super::s_collection[index]->componentId = componentId;
        return componentId;
    }

} // namespace ecs
