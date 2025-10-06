#include "ecs/component/StaticComponentsFactory.hpp"

#include <algorithm>
#include <array>

#include "../../include/ecs/component/ComponentError.hpp"


namespace ecs::component
{

    StaticComponentsFactory::StaticComponentsFactory() = default;

    StaticComponentsFactory::~StaticComponentsFactory() = default;

    StaticComponents&& StaticComponentsFactory::createComponent(const BaseComponent::ConditionArgs* args) const
    {
        bufferSize_t componentsSizeOf {0};
        componentId_t maxComponentId {INVALID_COMPONENT_ID};
        std::array<bool, OVERFLOW_MAX_COMPONENT_ID> conditionedComponents {false};

        // find componentsSizeOf, maxComponentId, fill conditionedComponents
        for (componentId_t componentId = 1; componentId <= m_maxRegisteredComponentId; ++componentId)
        {
            const StaticComponents::RegisterComponentInfo& registeredComponentInfo = m_registeredComponents[componentId];
            if (registeredComponentInfo.condition(args))
            {
                conditionedComponents[componentId] = true;
                componentsSizeOf += registeredComponentInfo.componentSize;
                maxComponentId = m_maxRegisteredComponentId;
            }
        }

        if (maxComponentId == INVALID_COMPONENT_ID)
            throw error::InvalidSizeComponents("m_maxComponentId == INVALID_COMPONENT_ID");

        // create StaticComponents and bind pointers
        StaticComponents components = {componentsSizeOf, maxComponentId};
        auto* bufferComponentInfo = reinterpret_cast<StaticComponents::ComponentInfo*>(components.m_buffer);
        auto* byteComponentData = reinterpret_cast<byte*>(bufferComponentInfo + maxComponentId);

        for (componentId_t componentId = 1; componentId <= maxComponentId; ++componentId)
        {
            if (!conditionedComponents[componentId])
                continue;

            const StaticComponents::RegisterComponentInfo& registeredComponentInfo = m_registeredComponents[componentId];

            // fill attr StaticComponents::ComponentInfo
            bufferComponentInfo->ptr = byteComponentData;
            bufferComponentInfo->registerComponentInfo = &registeredComponentInfo;

            byteComponentData += registeredComponentInfo.componentSize;
            ++bufferComponentInfo;
        }

        return std::move(components);
    }

    void StaticComponentsFactory::collectRegisterComponentInfo(StaticComponents::RegisterComponentInfo&& componentInfo)
    {
        const componentId_t componentId = componentInfo.componentId;
        if (componentId == INVALID_COMPONENT_ID)
            throw error::InvalidComponentId(
                "[collectRegisterComponentInfo] componentInfo.componentId: %u", componentId
                );

        StaticComponents::RegisterComponentInfo finedComponentInfo = m_registeredComponents[componentId];
        if (finedComponentInfo.componentId == INVALID_COMPONENT_ID)
            throw error::RepeatComponent(
                "[collectRegisterComponentInfo] finedComponentInfo.componentId == INVALID_COMPONENT_ID: %u", componentId
                );

        m_registeredComponents[componentId] = componentInfo;
        m_maxRegisteredComponentId = std::max(m_maxRegisteredComponentId, componentId);
    }

} // namespace ecs::component
