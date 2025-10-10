#include <algorithm>
#include <array>

#include "ecs/component/ComponentError.hpp"
#include "ecs/component/StaticComponentsFactory.hpp"


namespace ecs::component
{

    StaticComponentsFactory::StaticComponentsFactory() = default;

    StaticComponentsFactory::~StaticComponentsFactory() = default;

    StaticComponentsPtr StaticComponentsFactory::createComponents(const BaseComponent::ConditionArgs* args /* = nullptr */) const
    {
        StaticComponents* componentBuffer = nullptr;
        initComponentesData(componentBuffer, args);

        return StaticComponentsPtr(componentBuffer);
    }

    void StaticComponentsFactory::initComponentesData(StaticComponents*& componentBuffer, const BaseComponent::ConditionArgs* args) const
    {
        bufferSize_t componentsSizeOf {};
        componentId_t maxComponentId {};
        std::array<bool, OVERFLOW_MAX_COMPONENT_ID> conditionedComponents {false};

        // find componentsSizeOf, maxComponentId, fill conditionedComponents
        for (componentId_t componentId = 1; componentId <= m_maxRegisteredComponentId; ++componentId)
        {
            const StaticComponents::RegisterComponentInfo& registeredComponentInfo = m_registeredComponents[componentId];
            if (registeredComponentInfo.componentId != INVALID_COMPONENT_ID and registeredComponentInfo.condition(args))
            {
                conditionedComponents[componentId] = true;
                componentsSizeOf += registeredComponentInfo.componentSize;
                maxComponentId = m_maxRegisteredComponentId;
            }
        }

        // create StaticComponents and set pointers
        byte* byteBuffer = StaticComponents::newBuffer(componentsSizeOf, maxComponentId);
        new (byteBuffer) StaticComponents({maxComponentId});
        componentBuffer = reinterpret_cast<StaticComponents*>(byteBuffer);

        byteBuffer = byteBuffer + sizeof(StaticComponents);

        auto* bufferComponentInfo = reinterpret_cast<StaticComponents::ComponentInfo*>(byteBuffer);
        auto* byteComponentData = reinterpret_cast<byte*>(bufferComponentInfo + maxComponentId + 1);

        for (componentId_t componentId = 0; componentId <= maxComponentId; ++componentId)
        {
            if (!conditionedComponents[componentId])
                continue;

            const StaticComponents::RegisterComponentInfo& registeredComponentInfo = m_registeredComponents[componentId];

            // fill attr StaticComponents::ComponentInfo
            bufferComponentInfo[componentId].ptr = byteComponentData;
            bufferComponentInfo[componentId].registerComponentInfo = &registeredComponentInfo;

            byteComponentData += registeredComponentInfo.componentSize;
        }
    }

    void StaticComponentsFactory::collectRegisterComponentInfo(StaticComponents::RegisterComponentInfo&& componentInfo)
    {
        const componentId_t componentId = componentInfo.componentId;
        if (componentId == INVALID_COMPONENT_ID)
            throw error::InvalidComponentId(
                "[collectRegisterComponentInfo] componentInfo.componentId: %u", componentId
                );

        StaticComponents::RegisterComponentInfo finedComponentInfo = m_registeredComponents[componentId];
        if (finedComponentInfo.componentId != INVALID_COMPONENT_ID)
            throw error::RepeatComponent(
                "[collectRegisterComponentInfo] finedComponentInfo.componentId == INVALID_COMPONENT_ID: %u", componentId
                );

        m_registeredComponents[componentId] = componentInfo;
        m_maxRegisteredComponentId = std::max(m_maxRegisteredComponentId, componentId);
    }

} // namespace ecs::component
