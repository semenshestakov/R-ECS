#include <algorithm>
#include <array>
#include "ecs/components/ComponentsManager.hpp"
#include "ecs/utils/ComponentError.hpp"


ecs::ComponentsManager::ComponentsManager() = default;

ecs::ComponentsManager::~ComponentsManager() = default;

ecs::ComponentsManager::ComponentsManager(const ComponentsManager& other) { this->copy(other); }

ecs::ComponentsManager& ecs::ComponentsManager::operator=(const ComponentsManager& other)
{
    this->copy(other);
    return *this;
}

void ecs::ComponentsManager::copy(const ecs::ComponentsManager& other)
{
    std::ranges::copy(other.m_registeredComponents, std::begin(m_registeredComponents));
    m_maxRegisteredComponentId = other.m_maxRegisteredComponentId;
}

ecs::ComponentsManager::ComponentsManager(ComponentsManager&& other) noexcept { this->swap(other); }

ecs::ComponentsManager& ecs::ComponentsManager::operator=(ComponentsManager&& other) noexcept
{
    this->swap(other);
    return *this;
}

void ecs::ComponentsManager::swap(ecs::ComponentsManager& other) noexcept
{
    std::swap(m_registeredComponents, other.m_registeredComponents);
    std::swap(m_maxRegisteredComponentId, other.m_maxRegisteredComponentId);
}

void ecs::ComponentsManager::Register(const RegisterComponentInfo& componentInfo)
{
    const componentId_t componentId = componentInfo.componentId;
    if(componentId == INVALID_COMPONENT_ID)
        throw error::InvalidComponentId("[Register] componentInfo.componentId: %u", componentId);

    RegisterComponentInfo finedComponentInfo = m_registeredComponents[componentId];
    if(finedComponentInfo.componentId != INVALID_COMPONENT_ID)
        throw error::RepeatComponent("[Register] finedComponentInfo.componentId == INVALID_COMPONENT_ID: %u",
                                     componentId);

    m_registeredComponents[componentId] = componentInfo;
    m_maxRegisteredComponentId = std::max(m_maxRegisteredComponentId, componentId);
}

ecs::ComponentsPtr ecs::ComponentsManager::CreateComponents() const
{
    Components* componentBuffer = nullptr;
    initComponentesData(componentBuffer);

    return ComponentsPtr(componentBuffer);
}

void ecs::ComponentsManager::initComponentesData(Components*& componentBuffer) const
{
    bufferSize_t componentsSizeOf{};
    componentId_t maxComponentId{};
    std::array<bool, OVERFLOW_MAX_COMPONENT_ID> conditionedComponents{false};

    // find componentsSizeOf, maxComponentId, fill conditionedComponents
    for(componentId_t componentId = 1; componentId <= m_maxRegisteredComponentId; ++componentId)
    {
        const RegisterComponentInfo& registeredComponentInfo = m_registeredComponents[componentId];
        if(registeredComponentInfo.componentId != INVALID_COMPONENT_ID)
        {
            conditionedComponents[componentId] = true;
            componentsSizeOf += registeredComponentInfo.componentSize;
            maxComponentId = m_maxRegisteredComponentId;
        }
    }

    // create Components and set pointers
    byte* byteBuffer = Components::newBuffer(componentsSizeOf, maxComponentId);
    new(byteBuffer) Components({maxComponentId});
    componentBuffer = reinterpret_cast<Components*>(byteBuffer);

    byteBuffer = byteBuffer + sizeof(Components);

    auto* bufferComponentInfo = reinterpret_cast<Components::ComponentInfo*>(byteBuffer);
    auto* byteComponentData = reinterpret_cast<byte*>(bufferComponentInfo + maxComponentId + 1);

    for(componentId_t componentId = 0; componentId <= maxComponentId; ++componentId)
    {
        if(!conditionedComponents[componentId])
            continue;

        const RegisterComponentInfo& registeredComponentInfo = m_registeredComponents[componentId];

        // fill attr Components::ComponentInfo
        bufferComponentInfo[componentId].ptr = byteComponentData;
        bufferComponentInfo[componentId].registerComponentInfo = &registeredComponentInfo;

        byteComponentData += registeredComponentInfo.componentSize;
    }
}
